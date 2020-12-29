#include "assets_browser.h"
#include "halley/tools/project/project.h"
#include "halley/core/resources/resource_locator.h"
#include "halley/core/resources/standard_resources.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_list.h"
#include "animation_editor.h"
#include "asset_editor.h"
#include "asset_editor_window.h"
#include "metadata_editor.h"
#include "new_asset_window.h"
#include "prefab_editor.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file_formats/yaml_convert.h"
#include "src/ui/editor_ui_factory.h"

using namespace Halley;

AssetsBrowser::AssetsBrowser(EditorUIFactory& factory, Project& project, ProjectWindow& projectWindow)
	: UIWidget("assets_editor", {}, UISizer())
	, factory(factory)
	, project(project)
	, projectWindow(projectWindow)
	, curSrcPath(".")
	, fuzzyMatcher(false)
{
	loadResources();
	makeUI();
	setAssetSrcMode(true);
}

void AssetsBrowser::showAsset(AssetType type, const String& assetId)
{
	getWidgetAs<UITextInput>("assetSearch")->setText("");
	Path target;
	if (type == AssetType::Sprite) {
		auto ssAssetId = project.getGameResources().get<SpriteResource>(assetId)->getSpriteSheet()->getAssetId();
		target = project.getImportAssetsDatabase().getPrimaryInputFile(AssetType::SpriteSheet, ssAssetId);
	} else {
		target = project.getImportAssetsDatabase().getPrimaryInputFile(type, assetId);
	}
	showFile(target);
}

void AssetsBrowser::showFile(const Path& path)
{
	if (!path.isEmpty()) {
		curSrcPath = path.parentPath();
		refreshList();
		assetList->setSelectedOptionId(path.toString());
	}
}

void AssetsBrowser::loadResources()
{
	project.addAssetReloadCallback([=] (const std::vector<String>& assets)
	{
		refreshAssets(assets);
	});
}

void AssetsBrowser::makeUI()
{
	UIWidget::add(factory.makeUI("ui/halley/assets_browser"), 1);

	assetList = getWidgetAs<UIList>("assetList");
	assetList->setSingleClickAccept(false);
	assetEditor = getWidgetAs<AssetEditorWindow>("assetEditorWindow");
	assetEditor->init(project, projectWindow);
	getWidget("metadataPanel")->setActive(false);

	setHandle(UIEventType::ListSelectionChanged, "assetType", [=] (const UIEvent& event)
	{
		listAssets(fromString<AssetType>(event.getStringData()));
	});

	setHandle(UIEventType::ListSelectionChanged, "assetList", [=] (const UIEvent& event)
	{
		loadAsset(event.getStringData(), false);
	});

	setHandle(UIEventType::ListAccept, "assetList", [=] (const UIEvent& event)
	{
		loadAsset(event.getStringData(), true);
	});

	setHandle(UIEventType::TextChanged, "assetSearch", [=] (const UIEvent& event)
	{
		setFilter(event.getStringData());
	});

	setHandle(UIEventType::ButtonClicked, "addAsset", [=] (const UIEvent& event)
	{
		addAsset();
	});

	setHandle(UIEventType::ButtonClicked, "removeAsset", [=] (const UIEvent& event)
	{
		removeAsset();
	});

	setHandle(UIEventType::ButtonClicked, "openFile", [=] (const UIEvent& event)
	{
		openFileExternally(getCurrentAssetPath());
	});

	setHandle(UIEventType::ButtonClicked, "showFile", [=] (const UIEvent& event)
	{
		showFileExternally(getCurrentAssetPath());
	});

	updateAddRemoveButtons();
}

void AssetsBrowser::setAssetSrcMode(bool enabled)
{
	assetSrcMode = enabled;
	assetEditor->setAssetSrcMode(enabled);
	getWidget("assetType")->setActive(!assetSrcMode);
	if (assetSrcMode) {
		listAssetSources();
	} else {
		listAssets(AssetType::Sprite);
	}
}

void AssetsBrowser::listAssetSources()
{
	if (!assetNames) {
		assetNames = project.getAssetSrcList();
		std::sort(assetNames->begin(), assetNames->end()); // Is this even needed?
		fuzzyMatcher.clear();
		fuzzyMatcher.addStrings(assetNames.value());
	}

	if (filter.isEmpty()) {
		setListContents(assetNames.value(), curSrcPath, false);
	} else {
		std::vector<String> filteredList;
		auto result = fuzzyMatcher.match(filter);
		
		filteredList.reserve(result.size());
		for (const auto& r: result) {
			filteredList.push_back(r.str);
		}
		setListContents(filteredList, curSrcPath, true);
	}	
}

void AssetsBrowser::listAssets(AssetType type)
{
	curType = type;
	if (curPaths.find(type) == curPaths.end()) {
		curPaths[type] = Path(".");
	}
	const auto curPath = curPaths[type];

	auto assets = project.getGameResources().ofType(type).enumerate();
	std::sort(assets.begin(), assets.end());

	setListContents(assets, curPath, false);
}

void AssetsBrowser::setListContents(std::vector<String> assets, const Path& curPath, bool flat)
{
	{
		Hash::Hasher hasher;
		for (const auto& asset: assets) {
			hasher.feed(asset);
		}
		hasher.feed(curPath.toString());
		const auto hash = hasher.digest();

		if (curHash == hash) {
			return;
		}
		curHash = hash;
	}

	std::optional<String> selectOption;
	{
		Hash::Hasher hasher;
		hasher.feed(curPath.toString());
		const auto hash = hasher.digest();

		if (curDirHash == hash) {
			selectOption = assetList->getSelectedOptionId();
		}
		curDirHash = hash;
	}
	
	clearAssetList();

	if (flat) {
		for (auto& a: assets) {
			addFileToList(a);
		}
	} else {
		std::set<String> dirs;
		std::vector<String> files;

		for (auto& a: assets) {
			auto relPath = Path("./" + a).makeRelativeTo(curPath);
			if (relPath.getNumberPaths() == 1) {
				files.emplace_back(a);
			} else {
				auto start = relPath.getFront(1);
				dirs.insert(start.toString());
			}
		}

		for (const auto& dir: dirs) {
			addDirToList(dir);
		}
		for (const auto& file: files) {
			addFileToList(file);
		}
	}

	if (selectOption) {
		assetList->setSelectedOptionId(selectOption.value());
	}
}

void AssetsBrowser::clearAssetList()
{
	assetList->clear();
}

void AssetsBrowser::addDirToList(const String& dir)
{
	auto sizer = std::make_shared<UISizer>();
	sizer->add(std::make_shared<UIImage>(factory.makeDirectoryIcon(dir == "..")), 0, Vector4f(0, 0, 4, 0));
	sizer->add(assetList->makeLabel("", LocalisedString::fromUserString(dir)));
	assetList->addItem(dir + "/.", std::move(sizer));
}

void AssetsBrowser::addFileToList(const Path& path)
{
	auto type = project.getAssetImporter()->getImportAssetType(path, false);
	
	auto sizer = std::make_shared<UISizer>();
	sizer->add(std::make_shared<UIImage>(factory.makeImportAssetTypeIcon(type)), 0, Vector4f(0, 0, 4, 0));
	sizer->add(assetList->makeLabel("", LocalisedString::fromUserString(path.getFilename().toString())));
	assetList->addItem(path.toString(), std::move(sizer));
	
	//assetList->addTextItem(path.toString(), LocalisedString::fromUserString(path.getFilename().toString()));
}

void AssetsBrowser::refreshList()
{
	if (assetSrcMode) {
		listAssetSources();
	} else {
		listAssets(curType);
	}
}

void AssetsBrowser::setFilter(const String& f)
{
	if (filter != f) {
		filter = f.asciiLower();
		refreshList();
	}
}

void AssetsBrowser::loadAsset(const String& name, bool doubleClick)
{
	lastClickedAsset = name;
	updateAddRemoveButtons();
	
	auto& curPath = assetSrcMode ? curSrcPath : curPaths[curType];
	if (name.endsWith("/.")) {
		if (doubleClick) {
			curPath = curPath / name;
			refreshList();
		}
	} else {
		assetEditor->loadAsset(name, curType, true);

		if (doubleClick) {
			assetEditor->onDoubleClickAsset();
		}

		getWidget("metadataPanel")->setActive(true);
	}
}

void AssetsBrowser::refreshAssets(const std::vector<String>& assets)
{
	assetNames.reset();
	refreshList();
	assetEditor->refreshAssets();
}

void AssetsBrowser::updateAddRemoveButtons()
{
	// TODO: refactor updateAddRemoveButtons/addAsset/removeAsset?
	
	const auto stem = curSrcPath.getFront(1).string();
	const bool canAdd = stem == "prefab" || stem == "scene";
	const bool canRemove = !lastClickedAsset.isEmpty() && !lastClickedAsset.endsWith("/.");

	getWidget("addAsset")->setEnabled(canAdd);
	getWidget("removeAsset")->setEnabled(canRemove);
}

void AssetsBrowser::addAsset()
{
	// TODO: refactor updateAddRemoveButtons/addAsset/removeAsset?
	
	const auto assetType = curSrcPath.getFront(1).string();
	const auto dstPath = project.getAssetsSrcPath() / curSrcPath;

	getRoot()->addChild(std::make_shared<NewAssetWindow>(factory, [=] (std::optional<String> newName)
	{
		if (newName) {
			if (assetType == "prefab") {
				Prefab prefab;
				prefab.makeDefault();
				FileSystem::writeFile(dstPath / (newName.value() + ".prefab"), prefab.toYAML());
			} else if (assetType == "scene") {
				Scene scene;
				scene.makeDefault();
				FileSystem::writeFile(dstPath / (newName.value() + ".scene"), scene.toYAML());
			}
		}
	}));
}

void AssetsBrowser::removeAsset()
{
	// TODO: refactor updateAddRemoveButtons/addAsset/removeAsset?
	assetList->setItemActive(lastClickedAsset, false);
	FileSystem::remove(project.getAssetsSrcPath() / lastClickedAsset);
}

Path AssetsBrowser::getCurrentAssetPath() const
{
	return assetEditor->getCurrentAssetPath();
}

void AssetsBrowser::openFileExternally(const Path& path)
{
	auto cmd = "start \"\" \"" + path.toString().replaceAll("/", "\\") + "\"";
	system(cmd.c_str());
}

void AssetsBrowser::showFileExternally(const Path& path)
{
	auto cmd = "explorer.exe /select,\"" + path.toString().replaceAll("/", "\\") + "\"";
	system(cmd.c_str());
}
