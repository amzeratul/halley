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
#include "src/ui/project_window.h"

using namespace Halley;

AssetsBrowser::AssetsBrowser(EditorUIFactory& factory, Project& project, ProjectWindow& projectWindow)
	: UIWidget("assets_editor", {}, UISizer())
	, factory(factory)
	, project(project)
	, projectWindow(projectWindow)
	, curSrcPath(".")
	, fuzzyMatcher(false, 100)
{
	loadResources();
	makeUI();
	setAssetSrcMode(true);
}

void AssetsBrowser::openAsset(AssetType type, const String& assetId)
{
	getWidgetAs<UITextInput>("assetSearch")->setText("");
	auto target = project.getImportAssetsDatabase().getPrimaryInputFile(type, assetId);
	openFile(std::move(target));
}

void AssetsBrowser::openFile(const Path& path)
{
	if (!path.isEmpty()) {
		curSrcPath = path.parentPath();
		refreshList();
		assetList->setSelectedOptionId(path.toString());
		loadAsset(path.toString(), true);
	}
}

void AssetsBrowser::replaceAssetTab(AssetType oldType, const String& oldId, AssetType newType, const String& newId)
{
	const auto oldTarget = project.getImportAssetsDatabase().getPrimaryInputFile(oldType, oldId);
	const auto newTarget = project.getImportAssetsDatabase().getPrimaryInputFile(newType, newId);
	assetTabs->replaceAssetTab(oldTarget.toString(), newTarget.toString());
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

	assetTabs = std::make_shared<AssetBrowserTabs>(factory, project, projectWindow);
	getWidget("assetEditorContainer")->add(assetTabs, 1);

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

	setHandle(UIEventType::ButtonClicked, "collapseButton", [=] (const UIEvent& event)
	{
		setCollapsed(!collapsed);
	});

	updateAddRemoveButtons();

	doSetCollapsed(projectWindow.getSetting(EditorSettingType::Editor, "assetBrowserCollapse").asBool(false));
}

void AssetsBrowser::setAssetSrcMode(bool enabled)
{
	assetSrcMode = enabled;
	assetTabs->setAssetSrcMode(enabled);
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
			filteredList.push_back(r.getString());
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
			addDirToList(curPath, dir);
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

void AssetsBrowser::addDirToList(const Path& curPath, const String& dir)
{
	const auto icon = std::make_shared<UIImage>(factory.makeDirectoryIcon(dir == ".."));

	/*
	std::optional<ImportAssetType> assetTypeDir;
	if (curPath == ".") {
		if (dir == "shader") {
			assetTypeDir = ImportAssetType::Shader;
		}
	}
	if (assetTypeDir) {
		icon->add(std::make_shared<UIImage>(factory.makeImportAssetTypeIcon(assetTypeDir.value())), 1, {}, UISizerAlignFlags::Centre);
	}
	*/
	
	auto sizer = std::make_shared<UISizer>();
	sizer->add(icon, 0, Vector4f(0, 0, 4, 0));
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
		if (doubleClick) {
			assetTabs->load(assetSrcMode ? std::optional<AssetType>() : curType, name);
		}
	}
}

void AssetsBrowser::refreshAssets(const std::vector<String>& assets)
{
	assetNames.reset();
	refreshList();
	assetTabs->refreshAssets();
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

void AssetsBrowser::setCollapsed(bool collapsed)
{
	doSetCollapsed(collapsed);
	projectWindow.setSetting(EditorSettingType::Editor, "assetBrowserCollapse", ConfigNode(collapsed));
}

void AssetsBrowser::doSetCollapsed(bool c)
{
	if (collapsed != c) {
		collapsed = c;

		auto button = getWidgetAs<UIButton>("collapseButton");
		button->setLabel(LocalisedString::fromHardcodedString(collapsed ? ">>" : "<< Collapse"));

		auto* parent = dynamic_cast<UIWidget*>(button->getParent());
		if (parent) {
			parent->getSizer()[0].setBorder(collapsed ? Vector4f(-10, 0, -15, 0) : Vector4f(0, 0, 0, 5));
		}
		
		//getWidget("collapseBorder")->setActive(!collapsed);
		getWidget("assetBrowsePanel")->setActive(!collapsed);
	}
}
