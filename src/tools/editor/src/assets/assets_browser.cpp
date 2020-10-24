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
#include "halley/tools/yaml/yaml_convert.h"

using namespace Halley;

AssetsBrowser::AssetsBrowser(UIFactory& factory, Project& project, ProjectWindow& projectWindow)
	: UIWidget("assets_editor", {}, UISizer())
	, factory(factory)
	, project(project)
	, projectWindow(projectWindow)
	, curSrcPath(".")
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
	metadataEditor = getWidgetAs<MetadataEditor>("metadataEditor");
	assetEditor = getWidgetAs<AssetEditorWindow>("assetEditorWindow");
	assetEditor->init(project, projectWindow, metadataEditor);

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
	}

	if (filter.isEmpty()) {
		setListContents(assetNames.value(), curSrcPath, false);
	} else {
		std::vector<String> filteredList;
		for (auto& a: assetNames.value()) {
			if (a.asciiLower().contains(filter)) {
				filteredList.push_back(a);
			}
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
	
	assetList->clear();
	if (flat) {
		for (auto& a: assets) {
			assetList->addTextItem(a, LocalisedString::fromUserString(Path(a).getFilename().toString()));
		}
	} else {
		std::set<String> dirs;
		std::vector<std::pair<String, String>> files;

		for (auto& a: assets) {
			auto relPath = Path("./" + a).makeRelativeTo(curPath);
			if (relPath.getNumberPaths() == 1) {
				files.emplace_back(a, relPath.toString());
			} else {
				auto start = relPath.getFront(1);
				dirs.insert(start.toString());
			}
		}

		for (auto& dir: dirs) {
			assetList->addTextItem(dir + "/.", LocalisedString::fromUserString("[" + dir + "]"));
		}
		for (auto& file: files) {
			assetList->addTextItem(file.first, LocalisedString::fromUserString(file.second));
		}
	}

	if (selectOption) {
		assetList->setSelectedOptionId(selectOption.value());
	}
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
				FileSystem::writeFile(dstPath / (newName.value() + ".prefab"), YAMLConvert::generateYAML(prefab.getRoot(), YAMLConvert::EmitOptions()));
			} else if (assetType == "scene") {
				Scene scene;
				scene.makeDefault();
				FileSystem::writeFile(dstPath / (newName.value() + ".scene"), YAMLConvert::generateYAML(scene.getRoot(), YAMLConvert::EmitOptions()));
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
