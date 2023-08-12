#include "assets_browser.h"
#include "halley/tools/project/project.h"
#include "halley/resources/resource_locator.h"
#include "halley/resources/standard_resources.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_list.h"
#include "animation_editor.h"
#include "asset_editor.h"
#include "asset_editor_window.h"
#include "metadata_editor.h"
#include "new_asset_window.h"
#include "prefab_editor.h"
#include "halley/audio/audio_object.h"
#include "halley/tools/file/filesystem.h"
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

	getWidget("assetType")->setActive(false);
	listAssetSources();
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

bool AssetsBrowser::requestQuit(std::function<void()> callback)
{
	return assetTabs->requestQuit(std::move(callback));
}

void AssetsBrowser::saveTab()
{
	assetTabs->saveCurrentTab();
}

void AssetsBrowser::saveAllTabs()
{
	assetTabs->saveAllTabs();
}

void AssetsBrowser::closeTab()
{
	assetTabs->closeCurrentTab();
}

void AssetsBrowser::moveTabFocus(int delta)
{
	assetTabs->moveTabFocus(delta);
}

std::shared_ptr<AssetEditorWindow> AssetsBrowser::getActiveWindow() const
{
	return assetTabs->getActiveWindow();
}

void AssetsBrowser::loadResources()
{
	project.addAssetPackReloadCallback([=] (gsl::span<const String> assets, gsl::span<const String> packs)
	{
		refreshAssets(assets);
	});
}

void AssetsBrowser::makeUI()
{
	UIWidget::add(factory.makeUI("halley/assets_browser"), 1);

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

	setHandle(UIEventType::ListBackgroundRightClicked, "assetList", [=] (const UIEvent& event)
	{
		openContextMenu("");
	});

	setHandle(UIEventType::ListItemRightClicked, "assetList", [=] (const UIEvent& event)
	{
		openContextMenu(event.getStringData());
	});

	setHandle(UIEventType::TextChanged, "assetSearch", [=] (const UIEvent& event)
	{
		setFilter(event.getStringData());
	});

	setHandle(UIEventType::ButtonClicked, "addAsset", [=] (const UIEvent& event)
	{
		addAsset();
	});

	setHandle(UIEventType::ButtonClicked, "collapseButton", [=] (const UIEvent& event)
	{
		setCollapsed(!collapsed);
	});

	updateAddRemoveButtons();

	doSetCollapsed(projectWindow.getSetting(EditorSettingType::Editor, "assetBrowserCollapse").asBool(false));
}

void AssetsBrowser::listAssetSources()
{
	if (!assetNames) {
		assetNames = project.getAssetSrcList();
		refreshAssetNames();
	}

	if (filter.isEmpty()) {
		setListContents(assetNames.value(), curSrcPath, false);
	} else {
		Vector<String> filteredList;
		auto result = fuzzyMatcher.match(filter);
		
		filteredList.reserve(result.size());
		for (const auto& r: result) {
			filteredList.push_back(r.getString());
		}
		setListContents(filteredList, curSrcPath, true);
	}
}

void AssetsBrowser::refreshAssetNames()
{
	std::sort(assetNames->begin(), assetNames->end());
	fuzzyMatcher.clear();
	fuzzyMatcher.addStrings(assetNames.value());
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

void AssetsBrowser::setListContents(Vector<String> assets, const Path& curPath, bool flat)
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

	assetList->setScrollToSelection(false);
	clearAssetList();

	if (flat) {
		for (auto& a: assets) {
			addFileToList(a);
		}
	} else {
		std::set<String> dirs;
		Vector<String> files;

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
	assetList->setScrollToSelection(true);

	if (pendingOpen) {
		assetList->setSelectedOptionId(pendingOpen->toString());
		loadAsset(pendingOpen->toString(), true);
		pendingOpen.reset();
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
	auto id = assetList->getSelectedOptionId();
	assetList->setCanSendEvents(false);
	listAssetSources();
	assetList->setSelectedOptionId(id);
	assetList->setCanSendEvents(true);
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
	
	auto& curPath = curSrcPath;
	if (name.endsWith("/.")) {
		if (doubleClick) {
			curPath = curPath / name;
			refreshList();
		}
	} else {
		if (doubleClick) {
			assetTabs->load(name);
		}
	}
}

void AssetsBrowser::refreshAssets(gsl::span<const String> assets)
{
	assetNames.reset();
	refreshList();
	assetTabs->refreshAssets();
}

void AssetsBrowser::openContextMenu(const String& assetId)
{
	auto menuOptions = Vector<UIPopupMenuItem>();
	auto makeEntry = [&] (const String& id, const String& text, const String& toolTip, const String& icon, bool enabled = true)
	{
		auto iconSprite = Sprite().setImage(factory.getResources(), "entity_icons/" + (icon.isEmpty() ? "empty.png" : icon));
		menuOptions.push_back(UIPopupMenuItem(id, LocalisedString::fromHardcodedString(text), std::move(iconSprite), LocalisedString::fromHardcodedString(toolTip)));
		menuOptions.back().enabled = enabled;
	};

	const auto stem = curSrcPath.getFront(1).string();
	const bool canAdd = stem == "prefab" || stem == "scene" || stem == "audio_object" || stem == "audio_event" || stem == "ui" || stem == "comet";

	if (assetId.isEmpty()) {
		makeEntry("add", "Add", "Add new asset.", "add.png", canAdd);
	} else {
		const bool isDirectory = assetId.endsWith("/.");
		const bool isFile = !assetId.isEmpty() && !isDirectory;
		
		makeEntry("rename", "Rename", "Rename Asset.", "rename.png", isFile);
		makeEntry("duplicate", "Duplicate", "Duplicate Asset.", "duplicate.png", isFile && canAdd);
		makeEntry("delete", "Delete", "Delete Asset.", "delete.png", isFile);
	}

	auto menu = std::make_shared<UIPopupMenu>("asset_browser_context_menu", factory.getStyle("popupMenu"), menuOptions);
	menu->spawnOnRoot(*getRoot());

	menu->setHandle(UIEventType::PopupAccept, [this, assetId] (const UIEvent& e) {
		Concurrent::execute(Executors::getMainUpdateThread(), [=] () {
			onContextMenuAction(assetId, e.getStringData());
		});
	});
}

void AssetsBrowser::onContextMenuAction(const String& assetId, const String& action)
{
	const auto filename = Path(assetId).getFilename().replaceExtension("").toString();
	if (action == "add") {
		addAsset();
	} else if (action == "rename") {
		getRoot()->addChild(std::make_shared<NewAssetWindow>(factory, LocalisedString::fromHardcodedString("Rename asset to"), filename, [=](std::optional<String> newName)
		{
			if (newName) {
				const auto newPath = Path(assetId).parentPath() / Path(*newName).replaceExtension(Path(assetId).getExtension());
				renameAsset(assetId, newPath.toString());
			}
		}));
	} else if (action == "duplicate") {
		getRoot()->addChild(std::make_shared<NewAssetWindow>(factory, LocalisedString::fromHardcodedString("Enter name of new asset"), filename, [=](std::optional<String> newName)
		{
			if (newName) {
				const auto newPath = Path(assetId).parentPath() / Path(*newName).replaceExtension(Path(assetId).getExtension());
				duplicateAsset(assetId, newPath.toString());
			}
		}));
	} else if (action == "delete") {
		const auto buttons = Vector<UIConfirmationPopup::ButtonType>{ { UIConfirmationPopup::ButtonType::Yes, UIConfirmationPopup::ButtonType::No }};
		getRoot()->addChild(std::make_shared<UIConfirmationPopup>(factory, "Delete Asset?", "Are you sure you want to delete " + assetId + "?", buttons, [=](UIConfirmationPopup::ButtonType result)
		{
			if (result == UIConfirmationPopup::ButtonType::Yes) {
				removeAsset(assetId);
			}
		}));
	}
}

void AssetsBrowser::updateAddRemoveButtons()
{
	// TODO: refactor updateAddRemoveButtons/addAsset/removeAsset?
	
	const auto stem = curSrcPath.getFront(1).string();
	const bool canAdd = stem == "prefab" || stem == "scene" || stem == "audio_object" || stem == "audio_event" || stem == "ui" || stem == "comet";

	getWidget("addAsset")->setEnabled(canAdd);
}

void AssetsBrowser::addAsset()
{
	// TODO: refactor updateAddRemoveButtons/addAsset/removeAsset?
	
	const auto assetType = curSrcPath.getFront(1).string();

	getRoot()->addChild(std::make_shared<NewAssetWindow>(factory, LocalisedString::fromHardcodedString("New Asset"), "", [=](std::optional<String> newName)
	{
		if (newName) {
			if (assetType == "prefab") {
				Prefab prefab;
				prefab.makeDefault();
				addAsset(newName.value() + ".prefab", prefab.toYAML());
			}
			else if (assetType == "scene") {
				Scene scene;
				scene.makeDefault();
				addAsset(newName.value() + ".scene", scene.toYAML());
			}
			else if (assetType == "audio_object") {
				AudioObject object;
				object.makeDefault();
				addAsset(newName.value() + ".yaml", object.toYAML());
			}
			else if (assetType == "audio_event") {
				AudioEvent audioEvent;
				audioEvent.makeDefault();
				addAsset(newName.value() + ".yaml", audioEvent.toYAML());
			}
			else if (assetType == "ui") {
				UIDefinition ui;
				ui.makeDefault();
				addAsset(newName.value() + ".yaml", ui.toYAML());
			}
			else if (assetType == "comet") {
				ScriptGraph graph;
				graph.makeDefault();
				addAsset(newName.value() + ".comet", graph.toYAML());
			}
		}
	}));
}

void AssetsBrowser::addAsset(Path path, std::string_view data, bool isFullPath)
{
	const auto fullPath = isFullPath ? path : curSrcPath / path;
	pendingOpen = fullPath;
	project.writeAssetToDisk(fullPath, data);

	if (assetNames) {
		assetNames->push_back(fullPath.toString());
		refreshAssetNames();
	}
	refreshList();
}

void AssetsBrowser::removeAsset()
{
	removeAsset(lastClickedAsset);
}

void AssetsBrowser::removeAsset(const String& assetId)
{
	assetTabs->closeTab(assetId);
	FileSystem::remove(project.getAssetsSrcPath() / assetId);
	if (assetNames) {
		std_ex::erase(*assetNames, assetId);
		refreshAssetNames();
	}
	refreshList();
}

void AssetsBrowser::renameAsset(const String& oldName, const String& newName)
{
	//assetTabs->renameTab(oldName, newName, assetSrcMode ? std::optional<AssetType>() : curType);
	assetTabs->closeTab(oldName);
	FileSystem::rename(project.getAssetsSrcPath() / oldName, project.getAssetsSrcPath() / newName);
	if (assetNames) {
		std_ex::erase(*assetNames, oldName);
		assetNames->push_back(newName);
		refreshAssetNames();
	}
	refreshList();
}

void AssetsBrowser::duplicateAsset(const String& srcId, const String& dstId)
{
	const auto assetType = curSrcPath.getFront(1).string();
	auto srcFile = Path::readFile(project.getAssetsSrcPath() / srcId);
	if (srcFile.empty()) {
		return;
	}

	const auto configRoot = YAMLConvert::parseConfig(srcFile);
	const auto& configNode = configRoot.getRoot();

	if (assetType == "prefab") {
		Prefab prefab;
		prefab.parseConfigNode(configNode);
		prefab.generateUUIDs();
		addAsset(dstId, prefab.toYAML(), true);
	}
	else if (assetType == "scene") {
		Scene scene;
		scene.parseConfigNode(configNode);
		scene.generateUUIDs();
		addAsset(dstId, scene.toYAML(), true);
	}
	else if (assetType == "audio_object") {
		auto object = AudioObject(configNode);
		addAsset(dstId, object.toYAML(), true);
	}
	else if (assetType == "audio_event") {
		auto audioEvent = AudioEvent(configNode);
		addAsset(dstId, audioEvent.toYAML(), true);
	}
	else if (assetType == "ui") {
		auto ui = UIDefinition(ConfigNode(configNode));
		addAsset(dstId, ui.toYAML(), true);
	}
	else if (assetType == "comet") {
		auto graph = ScriptGraph(configNode);
		addAsset(dstId, graph.toYAML(), true);
	}
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
			parent->getSizer()[0].setBorder(collapsed ? Vector4f(-10, 0, -15, 0) : Vector4f(0, 0, 6, 0));
		}
		
		//getWidget("collapseBorder")->setActive(!collapsed);
		getWidget("assetBrowsePanel")->setActive(!collapsed);
	}
}
