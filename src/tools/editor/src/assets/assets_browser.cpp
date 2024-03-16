#include "assets_browser.h"
#include "halley/tools/project/project.h"
#include "halley/ui/widgets/ui_list.h"
#include "asset_editor_window.h"
#include "asset_file_handler.h"
#include "new_asset_window.h"
#include "prefab_editor.h"
#include "halley/tools/file/filesystem.h"
#include "src/ui/editor_ui_factory.h"
#include "src/ui/project_window.h"

using namespace Halley;

AssetsBrowser::AssetsBrowser(EditorUIFactory& factory, Project& project, ProjectWindow& projectWindow)
	: UIWidget("assets_editor", {}, UISizer())
	, factory(factory)
	, project(project)
	, projectWindow(projectWindow)
	, assetFileHandler(std::make_unique<AssetFileHandler>())
	, curSrcPath(".")
{
	makeUI();
	listAssetSources();

	project.addAssetSrcChangeListener(*this);
}

AssetsBrowser::~AssetsBrowser()
{}

void AssetsBrowser::update(Time t, bool moved)
{
	if (waitingToShowSel) {
		if (--waitingToShowSel == 0) {
			assetList->showCurSelection(true);
		}
	}
}

void AssetsBrowser::openAsset(AssetType type, const String& assetId)
{
	const auto target = project.getImportAssetsDatabase().getPrimaryInputFile(type, assetId);
	openFile(target);
}

void AssetsBrowser::openFile(const Path& path)
{
	if (!path.isEmpty()) {
		showFile(path);
		loadAsset(path.toString());
	}
}

void AssetsBrowser::showFile(const Path& path)
{
	if (!path.isEmpty()) {
		curSrcPath = path.parentPath();
		refreshList();
		assetList->setSelectedOptionId(path.toString());
		waitingToShowSel = 2;
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

void AssetsBrowser::onAssetsSrcChanged()
{
	refreshList();
}

void AssetsBrowser::makeUI()
{
	UIWidget::add(factory.makeUI("halley/assets_browser"), 1);

	assetList = getWidgetAs<UIList>("assetList");
	assetList->setSingleClickAccept(false);

	assetTabs = std::make_shared<AssetBrowserTabs>(factory, project, projectWindow);
	getWidget("assetEditorContainer")->add(assetTabs, 1);

	setHandle(UIEventType::ListSelectionChanged, "assetList", [=] (const UIEvent& event)
	{
		setSelectedAsset(event.getStringData());
	});

	setHandle(UIEventType::ListAccept, "assetList", [=] (const UIEvent& event)
	{
		loadAsset(event.getStringData());
	});

	setHandle(UIEventType::ListBackgroundRightClicked, "assetList", [=] (const UIEvent& event)
	{
		openContextMenu("");
	});

	setHandle(UIEventType::ListItemRightClicked, "assetList", [=] (const UIEvent& event)
	{
		openContextMenu(event.getStringData());
	});
	
	setHandle(UIEventType::ButtonClicked, "addAsset", [=] (const UIEvent& event)
	{
		addAsset();
	});
	
	setHandle(UIEventType::ButtonClicked, "addFolder", [=] (const UIEvent& event)
	{
		addFolder();
	});
	
	setHandle(UIEventType::ButtonClicked, "goToAssetButton", [=] (const UIEvent& event)
	{
		showFile(assetTabs->getCurrentAssetId());
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
	assetNames = project.getAssetSrcList(true, curSrcPath, false);
	if (curSrcPath != Path(".")) {
		assetNames.push_back((project.getAssetsSrcPath() / curSrcPath / "..").toString());
	}
	std::sort(assetNames.begin(), assetNames.end());
	
	setListContents();
}

void AssetsBrowser::setListContents()
{
	{
		Hash::Hasher hasher;
		for (const auto& asset: assetNames) {
			hasher.feed(asset);
		}
		hasher.feed(curSrcPath.toString());
		const auto hash = hasher.digest();

		if (curHash == hash) {
			return;
		}
		curHash = hash;
	}

	std::optional<String> selectOption;
	{
		Hash::Hasher hasher;
		hasher.feed(curSrcPath.toString());
		const auto hash = hasher.digest();

		if (curDirHash == hash) {
			selectOption = assetList->getSelectedOptionId();
		}
		curDirHash = hash;
	}

	assetList->setScrollToSelection(false);
	clearAssetList();

	std::set<String> dirs;
	Vector<String> files;

	for (auto& a: assetNames) {
		auto relPath = Path("./" + a).makeRelativeTo(curSrcPath);
		if (relPath.getNumberPaths() == 1) {
			files.emplace_back(a);
		} else {
			auto start = relPath.getFront(1);
			dirs.insert(start.toString());
		}
	}

	for (const auto& dir: dirs) {
		addDirToList(curSrcPath, dir);
	}
	for (const auto& file: files) {
		addFileToList(file);
	}

	if (selectOption) {
		assetList->setSelectedOptionId(selectOption.value());
	}
	assetList->setScrollToSelection(true);

	const auto pathStr = curSrcPath.getString(false);
	getWidgetAs<UILabel>("curDir")->setText(LocalisedString::fromUserString(pathStr.isEmpty() ? "assets_src" : pathStr));

	if (pendingOpen) {
		assetList->setSelectedOptionId(pendingOpen->toString());
		loadAsset(pendingOpen->toString());
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
	
	auto sizer = std::make_shared<UISizer>();
	sizer->add(icon, 0, Vector4f(0, 0, 4, 0));
	sizer->add(assetList->makeLabel("", LocalisedString::fromUserString(dir)));
	assetList->addItem(curPath / dir + "/.", std::move(sizer));
}

void AssetsBrowser::addFileToList(const Path& path)
{
	auto type = project.getAssetImporter()->getImportAssetType(path, false);
	
	auto sizer = std::make_shared<UISizer>();
	sizer->add(std::make_shared<UIImage>(factory.makeImportAssetTypeIcon(type)), 0, Vector4f(0, 0, 4, 0));
	sizer->add(assetList->makeLabel("", LocalisedString::fromUserString(path.getFilename().toString())));
	assetList->addItem(path.toString(), std::move(sizer));
}

void AssetsBrowser::refreshList()
{
	auto id = assetList->getSelectedOptionId();
	assetList->setCanSendEvents(false);
	listAssetSources();
	assetList->setSelectedOptionId(id);
	assetList->setCanSendEvents(true);
	updateAddRemoveButtons();
}

void AssetsBrowser::setSelectedAsset(const String& name)
{
	lastClickedAsset = name;
}

void AssetsBrowser::loadAsset(const String& name)
{
	if (name.endsWith("/.")) {
		curSrcPath = name;
		refreshList();
	} else {
		assetTabs->load(name);
	}
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
	const bool canAdd = assetFileHandler->canAdd(stem);
	const bool canDuplicate = assetFileHandler->canDuplicate(stem);

	if (assetId.isEmpty()) {
		makeEntry("add", "New asset...", "Create new asset.", "new_file.png", canAdd);
		makeEntry("addFolder", "New folder...", "Create new folder.", "new_folder.png", true);
	} else {
		const bool isDirectory = assetId.endsWith("/.");

		if (isDirectory) {
			makeEntry("renameFolder", "Rename", "Rename folder.", "rename.png", true);
			makeEntry("deleteFolder", "Delete", "Delete folder.", "delete.png", true);
		} else {
			makeEntry("rename", "Rename", "Rename Asset.", "rename.png", true);
			makeEntry("duplicate", "Duplicate", "Duplicate Asset.", "duplicate.png", canDuplicate);
			makeEntry("delete", "Delete", "Delete Asset.", "delete.png", true);
		}
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
	const auto filename = Path(assetId).getFilename().replaceExtension("").getString(false);
	if (action == "add") {
		addAsset();
	} else if (action == "rename") {
		const auto extension = Path(assetId).getExtension();
		getRoot()->addChild(std::make_shared<NewAssetWindow>(factory, LocalisedString::fromHardcodedString("Rename asset to"), filename, extension, [=](std::optional<String> newName)
		{
			if (newName) {
				const auto newPath = Path(assetId).parentPath() / Path(*newName).replaceExtension(extension);
				renameAsset(assetId, newPath.toString());
			}
		}));
	} else if (action == "duplicate") {
		const auto extension = Path(assetId).getExtension();
		getRoot()->addChild(std::make_shared<NewAssetWindow>(factory, LocalisedString::fromHardcodedString("Enter name of new asset"), filename, extension, [=](std::optional<String> newName)
		{
			if (newName) {
				const auto newPath = Path(assetId).parentPath() / Path(*newName).replaceExtension(extension);
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
	} else if (action == "addFolder") {
		addFolder();
	} else if (action == "renameFolder") {
		const auto dirName = Path(assetId).getDirNameStr();
		getRoot()->addChild(std::make_shared<NewAssetWindow>(factory, LocalisedString::fromHardcodedString("Rename folder to"), dirName, "", [=](std::optional<String> newName)
		{
			if (newName) {
				const auto newPath = Path(assetId).parentPath() / Path(*newName);
				renameFolder(assetId, newPath.toString());
			}
		}));
	} else if (action == "deleteFolder") {
		const auto buttons = Vector<UIConfirmationPopup::ButtonType>{ { UIConfirmationPopup::ButtonType::Yes, UIConfirmationPopup::ButtonType::No }};
		getRoot()->addChild(std::make_shared<UIConfirmationPopup>(factory, "Delete folder?", "Are you sure you want to delete " + assetId + "?", buttons, [=](UIConfirmationPopup::ButtonType result)
		{
			if (result == UIConfirmationPopup::ButtonType::Yes) {
				removeFolder(assetId);
			}
		}));
	}
}

void AssetsBrowser::updateAddRemoveButtons()
{
	const auto stem = curSrcPath.getFront(1).string();
	const bool canAdd = assetFileHandler->canAdd(stem);

	getWidget("addAsset")->setEnabled(canAdd);
}

void AssetsBrowser::addAsset()
{
	const auto assetType = curSrcPath.getFront(1).string();
	auto* handler = assetFileHandler->tryGetHandlerFor(assetType);
	if (!handler) {
		Logger::logError("Not handler found for assetType " + assetType);
		return;
	}
	const String extension = handler->getFileExtension();
	const String typeName = handler->getName();

	getRoot()->addChild(std::make_shared<NewAssetWindow>(factory, LocalisedString::fromHardcodedString("New " + typeName), "", extension, [=](std::optional<String> newName)
	{
		if (newName) {
			addAsset(newName.value() + extension, handler->makeDefaultFile());
		}
	}));
}

void AssetsBrowser::duplicateAsset(const String& srcId, const String& dstId)
{
	const auto assetType = curSrcPath.getFront(1).string();
	const auto* handler = assetFileHandler->tryGetHandlerFor(assetType);
	if (!handler) {
		Logger::logError("Not handler found for assetType " + assetType);
		return;
	}

	const auto srcFile = Path::readFile(project.getAssetsSrcPath() / srcId);
	if (srcFile.empty()) {
		return;
	}

	const auto configRoot = YAMLConvert::parseConfig(srcFile);
	const auto& configNode = configRoot.getRoot();

	addAsset(dstId, handler->duplicateAsset(configNode), true);
}

void AssetsBrowser::addAsset(Path path, std::string_view data, bool isFullPath)
{
	const auto fullPath = isFullPath ? path : curSrcPath / path;
	pendingOpen = fullPath;
	project.writeAssetToDisk(fullPath, data);

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
	refreshList();
}

void AssetsBrowser::renameAsset(const String& oldName, const String& newName)
{
	assetTabs->closeTab(oldName);
	FileSystem::rename(project.getAssetsSrcPath() / oldName, project.getAssetsSrcPath() / newName);
	refreshList();
}

void AssetsBrowser::removeFolder(const String& assetId)
{
	FileSystem::remove(project.getAssetsSrcPath() / assetId);
	refreshList();
}

void AssetsBrowser::renameFolder(const String& oldName, const String& newName)
{
	FileSystem::rename(project.getAssetsSrcPath() / oldName, project.getAssetsSrcPath() / newName);
	refreshList();
}

void AssetsBrowser::addFolder()
{
	getRoot()->addChild(std::make_shared<NewAssetWindow>(factory, LocalisedString::fromHardcodedString("New Folder"), "", "", [=](std::optional<String> newName)
	{
		addFolder(project.getAssetsSrcPath() / curSrcPath / *newName / ".");
	}));
}

void AssetsBrowser::addFolder(Path path)
{
	FileSystem::createDir(path);
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
		
		getWidget("assetBrowsePanel")->setActive(!collapsed);
	}
}
