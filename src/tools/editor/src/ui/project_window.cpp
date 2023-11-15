#include "project_window.h"


#include "console_window.h"
#include "ecs_window.h"
#include "editor_settings_window.h"
#include "game_properties_window.h"
#include "status_bar.h"
#include "taskbar.h"
#include "halley/tools/project/project.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/tools/dll/load_dll_task.h"
#include "halley/tools/project/build_project_task.h"
#include "halley/tools/project/project_properties.h"
#include "src/editor_root_stage.h"
#include "src/halley_editor.h"
#include "src/assets/assets_browser.h"
#include "src/assets/asset_editor_window.h"
#include "src/project/check_source_update_task.h"
#include "src/project/check_update_task.h"
#include "src/scene/choose_window.h"
#include "src/scene/scene_editor_window.h"
#include "src/ui/editor_ui_factory.h"

using namespace Halley;

ProjectWindow::ProjectWindow(EditorUIFactory& factory, HalleyEditor& editor, Project& project, Resources& resources, const HalleyAPI& api)
	: UIWidget("project_window", {}, UISizer(UISizerType::Vertical))
	, factory(factory)
	, editor(editor)
	, project(project)
	, resources(resources)
	, api(api)
{
	settings[EditorSettingType::Temp] = std::make_unique<SettingsStorage>(std::shared_ptr<ISaveData>(), "");
	settings[EditorSettingType::Project] = std::make_unique<SettingsStorage>(api.system->getStorageContainer(SaveDataType::SaveLocal, "settings"), project.getProperties().getUUID().toString());
	settings[EditorSettingType::Editor] = std::make_unique<SettingsStorage>(api.system->getStorageContainer(SaveDataType::SaveLocal, "settings"), "halleyEditor");

	tasks = std::make_unique<TaskSet>();

	entityEditorFactoryRoot = std::make_shared<EntityEditorFactoryRoot>(*this, factory);
	entityEditorFactoryRoot->addStandardFieldFactories();
	entityEditorFactoryRoot->setGameResources(project.getGameResources(), api);

	project.withDLL([&] (ProjectDLL& dll)
	{
		dll.addReloadListener(*this);
		updateDLLStatus(dll.getStatus());
		hasDLL = dll.isLoaded();
	});
	project.addAssetLoadedListener(this);

	tasks->addTask(std::make_unique<CheckAssetsTask>(project, false));
	tasks->addTask(std::make_unique<CheckUpdateTask>(*this, project.getRootPath()));
	tasks->addTask(std::make_unique<CheckSourceUpdateTask>(*this, project.getRootPath()));
}

ProjectWindow::~ProjectWindow()
{
	project.withDLL([&] (ProjectDLL& dll)
	{
		dll.removeReloadListener(*this);
	});
	project.removeAssetLoadedListener(this);
}

void ProjectWindow::onRemovedFromRoot(UIRoot& root)
{
	destroyConsole();
	consoleWindow.reset();

	assetPreviewGenerator.reset();
	assetEditorWindow.reset();
	assetFinder.reset();
}

void ProjectWindow::makeUI()
{
	uiTop = std::make_shared<UIWidget>("uiTop", Vector2f(), UISizer(UISizerType::Horizontal));
	uiMid = std::make_shared<UIWidget>("uiMid", Vector2f(), UISizer(UISizerType::Horizontal));
	uiBottom = std::make_shared<UIWidget>("uiBottom", Vector2f(), UISizer(UISizerType::Vertical));
	add(uiTop);
	add(uiMid, 1);
	add(uiBottom);

	makeToolbar();
	makePagedPane();

	auto statusBar = std::make_shared<StatusBar>(factory, *this);
	uiBottom->add(std::make_shared<TaskBar>(factory, *tasks, api), 1, Vector4f(0, 0, 0, 0));
	uiBottom->add(statusBar, 0, Vector4f(8, 0, 8, 4));

	consoleWindow->setStatusBar(statusBar);

	setHandle(UIEventType::NavigateToAsset, [=] (const UIEvent& event)
	{
		auto uri = event.getStringData();
		if (uri.startsWith("asset:")) {
			auto splitURI = uri.split(':', 3);
			if (splitURI.size() == 3) {
				openAsset(fromString<AssetType>(splitURI.at(1)), splitURI.at(2), true);
			}
		}
	});

	setHandle(UIEventType::NavigateToFile, [=] (const UIEvent& event)
	{
		auto uri = event.getStringData();
		if (uri.startsWith("asset:")) {
			auto splitURI = uri.split(':', 3);
			if (splitURI.size() == 3) {
				openAsset(fromString<AssetType>(splitURI.at(1)), splitURI.at(2), false);
			}
		}
	});

	setModal(false);
}

void ProjectWindow::makeToolbar()
{
	if (toolbar) {
		toolbar->destroy();
		uiTop->clear();
	}
	
	toolbar = std::make_shared<Toolbar>(factory, *this, project);
	
	uiTop->add(toolbar, 1, Vector4f(0, 8, 0, 0));
}

void ProjectWindow::makePagedPane()
{
	if (pagedPane) {
		pagedPane->destroy();
		uiMid->clear();
	}

	assetEditorWindow = std::make_shared<AssetsBrowser>(factory, project, *this);
	consoleWindow = std::make_shared<ConsoleWindow>(factory, api);
	auto remotes = std::make_shared<UIWidget>();
	auto settings = std::make_shared<EditorSettingsWindow>(factory, editor.getPreferences(), project, editor.getProjectLoader(), *this);
	auto properties = std::make_shared<GamePropertiesWindow>(factory, project);
	auto ecs = std::make_shared<ECSWindow>(factory, project);

	const auto margin = Vector4f(8, 8, 8, 4);
	pagedPane = std::make_shared<UIPagedPane>("pages", numOfStandardTools);
	pagedPane->setGuardedUpdate(true);
	pagedPane->getPage(static_cast<int>(EditorTabs::Assets))->add(assetEditorWindow, 1, margin);
	pagedPane->getPage(static_cast<int>(EditorTabs::ECS))->add(ecs, 1, margin);
	pagedPane->getPage(static_cast<int>(EditorTabs::Remotes))->add(remotes, 1, margin);
	pagedPane->getPage(static_cast<int>(EditorTabs::Properties))->add(properties, 1, margin);
	pagedPane->getPage(static_cast<int>(EditorTabs::Settings))->add(settings, 1, margin);
	pagedPane->getPage(static_cast<int>(EditorTabs::Terminal))->add(consoleWindow, 1, margin);

	uiMid->add(pagedPane, 1);
}

void ProjectWindow::tryLoadCustomUI()
{
	if (waitingToLoadCustomUI && hasAssets && hasDLL) {
		if (loadCustomUI()) {
			waitingToLoadCustomUI = false;
		}
	}
}

bool ProjectWindow::loadCustomUI()
{
	destroyCustomUI();

	auto game = project.getGameInstance();
	if (!game) {
		return false;
	}
	
	auto customToolsInterface = game->createEditorCustomToolsInterface();
	if (customToolsInterface) {
		try {
			customTools = customToolsInterface->makeTools(IEditorCustomTools::MakeToolArgs(factory, resources, project.getGameResources(), api, project, *this));
		} catch (const std::exception& e) {
			Logger::logException(e);
		} catch (...) {}

		if (!customTools.empty()) {
			toolbar->getList()->add(std::make_shared<UIImage>(Sprite().setImage(resources, "ui/slant_capsule_short.png").setColour(factory.getColourScheme()->getColour("toolbarNormal"))), 0, Vector4f(0, 3, 0, 3));
			
			for (auto& tool: customTools) {
				const auto img = std::make_shared<UIImage>(tool.icon);
				toolbar->getList()->addImage(tool.id, img, 1, {}, UISizerAlignFlags::Centre);
				toolbar->getList()->getItem(tool.id)->setToolTip(tool.tooltip);
				pagedPane->addPage()->add(tool.widget, 1, Vector4f(8, 8, 8, 8));
			}
		}
	}

	setupConsole(*game);

	entityEditorFactoryRoot->clear();
	entityEditorFactoryRoot->addStandardFieldFactories();
	entityEditorFactoryRoot->addFieldFactories(project.getGameInstance()->createCustomEditorFieldFactories(project.getGameResources()));
	
	return true;
}

void ProjectWindow::destroyCustomUI()
{
	entityEditorFactoryRoot->clear();

	if (!customTools.empty()) {
		makeToolbar();
		customTools.clear();
	}
	pagedPane->resizePages(numOfStandardTools);

	destroyConsole();
}

void ProjectWindow::onProjectDLLStatusChange(ProjectDLL::Status status)
{
	if (status == ProjectDLL::Status::Loaded) {
		hasDLL = true;
		waitingToLoadCustomUI = true;
		tryLoadCustomUI();
	} else {
		destroyCustomUI();
		for (const auto& ss: resources.enumerate<SpriteSheet>()) {
			resources.get<SpriteSheet>(ss)->clearMaterialCache();
		}
		for (const auto& ss: project.getGameResources().enumerate<SpriteSheet>()) {
			project.getGameResources().get<SpriteSheet>(ss)->clearMaterialCache();
		}
		project.clearCachedAssetPreviews();
		assetPreviewGenerator = {};
		getRoot()->releaseWeakPtrs();
	}

	updateDLLStatus(status);
}

void ProjectWindow::onAssetsLoaded()
{
	hasAssets = true;
	tryLoadCustomUI();
	for (auto& c: customTools) {
		c.widget->sendEvent(UIEvent(UIEventType::AssetsReloaded, getId()));
	}
	project.withLoadedDLL([&](ProjectDLL& dll) {
		entityEditorFactoryRoot->addFieldFactories(dll.getGame().createCustomEditorFieldFactories(project.getGameResources()));
	});
}

void ProjectWindow::update(Time t, bool moved)
{
	if (tasks) {
		tasks->update(t);
	}

	timeSinceSettingsSaved += t;
	if (timeSinceSettingsSaved >= 5) {
		for (auto& s: settings) {
			s.second->save();
		}
		timeSinceSettingsSaved = 0;
	}

	const auto size = api.video->getWindow().getDefinition().getSize();
	setMinSize(Vector2f(size));
}

bool ProjectWindow::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::P, KeyMods::Ctrl)) {
		openAssetFinder("");
		return true;
	}

	if (assetEditorWindow && assetEditorWindow->isActiveInHierarchy()) {
		if (key.is(KeyCode::S, KeyMods::Ctrl)) {
			assetEditorWindow->saveTab();
			return true;
		}

		if (key.is(KeyCode::S, KeyMods::CtrlShift)) {
			assetEditorWindow->saveAllTabs();
			return true;
		}

		if (key.is(KeyCode::W, KeyMods::Ctrl)) {
			assetEditorWindow->closeTab();
			return true;
		}

		if (key.is(KeyCode::Tab, KeyMods::Ctrl)) {
			assetEditorWindow->moveTabFocus(1);
			return true;
		}

		if (key.is(KeyCode::Tab, KeyMods::CtrlShift)) {
			assetEditorWindow->moveTabFocus(-1);
			return true;
		}
	}

	if (key.is(KeyCode::F1)) {
		toggleDebugConsole();
		return true;
	}
	
	return false;
}

void ProjectWindow::setPage(EditorTabs tab)
{
	pagedPane->setPage(static_cast<int>(tab));
	toolbar->onPageSet(toString(tab));
}

LocalisedString ProjectWindow::setCustomPage(const String& pageId)
{
	int i = 0;
	for (auto& custom: customTools) {
		if (custom.id == pageId) {
			pagedPane->setPage(numOfStandardTools + i);
			toolbar->onPageSet(pageId);
			return custom.text;
		}
		++i;
	}
	return LocalisedString::fromHardcodedString("???");
}

void ProjectWindow::openFile(const String& assetId)
{
	toolbar->getList()->setSelectedOptionId(toString(EditorTabs::Assets));
	assetEditorWindow->openFile(assetId);
}

void ProjectWindow::showFile(const String& assetId)
{
	toolbar->getList()->setSelectedOptionId(toString(EditorTabs::Assets));
	assetEditorWindow->showFile(assetId);
}

void ProjectWindow::openAsset(AssetType type, const String& assetId, bool inEditor)
{
	if (inEditor) {
		toolbar->getList()->setSelectedOptionId(toString(EditorTabs::Assets));
		assetEditorWindow->openAsset(type, assetId);
	} else {
		auto path = project.getAssetsSrcPath() / project.getImportAssetsDatabase().getPrimaryInputFile(type, assetId);
		openFileExternally(path);
	}
}

void ProjectWindow::replaceAssetTab(AssetType oldType, const String& oldId, AssetType newType, const String& newId)
{
	toolbar->getList()->setSelectedOptionId(toString(EditorTabs::Assets));
	assetEditorWindow->replaceAssetTab(oldType, oldId, newType, newId);
}

const HalleyAPI& ProjectWindow::getAPI() const
{
	return api;
}

Project& ProjectWindow::getProject()
{
	return project;
}

TaskSet& ProjectWindow::getTasks() const
{
	return *tasks;
}

void ProjectWindow::openAssetFinder(std::optional<String> initialQuery)
{
	if (!assetFinder) {
		assetFinder = std::make_shared<PaletteWindow>(factory, project, initialQuery, [=] (std::optional<String> result)
		{
			if (result) {
				openFile(result.value());
			}
			assetFinder.reset();
		});

		assetFinder->setInputGhostText(LocalisedString::fromHardcodedString("Search files by name"));
		if (const auto curAssetEditor = assetEditorWindow->getActiveWindow()) {
			curAssetEditor->onOpenAssetFinder(*assetFinder);
		}

		getRoot()->addChild(assetFinder);
	}
}

UIDebugConsoleController* ProjectWindow::getDebugConsoleController()
{
	return debugConsoleController.get();
}

Preferences& ProjectWindow::getPreferences() const
{
	return editor.getPreferences();
}

void ProjectWindow::toggleDebugConsole()
{
	if (debugConsole && debugConsole->isActive()) {
		debugConsole->hide();
	} else {
		if (debugConsoleController) { // Don't create unless ready
			if (!debugConsole) {
				debugConsole = std::make_shared<UIDebugConsole>("debugConsole", factory, debugConsoleController);
				debugConsole->setChildLayerAdjustment(50);
				debugConsole->setMinSize(Vector2f(640, 320));
				debugConsole->setAnchor(UIAnchor(Vector2f(1.0f, 1.0f), Vector2f(1.0f, 1.0f), Vector2f(-10.0f, -10.0f)));
				getRoot()->addChild(debugConsole);
			}
			debugConsole->show();
		}
	}
}

void ProjectWindow::updateDLLStatus(ProjectDLL::Status status)
{
	if (status != ProjectDLL::Status::Unloaded) {
		if (!firstDLLLoad || status != ProjectDLL::Status::Loaded) {
			addTask(std::make_unique<LoadDLLTask>(*this, status));
		}
	}

	firstDLLLoad = false;
}

void ProjectWindow::reloadDLL()
{
	Concurrent::execute(Executors::getMainUpdateThread(), [=]() {
		project.withDLL([&](ProjectDLL& dll)
		{
			dll.reload();
		});
	});
	if (debugConsole) {
		debugConsole->hide();
	}
}

void ProjectWindow::setupConsole(Game& game)
{
	debugConsoleController = std::make_shared<UIDebugConsoleController>(resources, api);
	debugConsoleCommands = std::make_shared<UIDebugConsoleCommands>();
	debugConsoleCommands->addCommand("dllReload", [=](Vector<String> args) -> String
	{
		reloadDLL();
		return "Reloading DLL";
	});
	try {
		game.attachToEditorDebugConsole(*debugConsoleCommands, project.getGameResources(), project);
		debugConsoleController->addCommands(*debugConsoleCommands);
	} catch (const std::exception& e) {
		Logger::logException(e);
	} catch (...) {}
}

void ProjectWindow::destroyConsole()
{
	if (debugConsole) {
		debugConsole->destroy();
		debugConsole.reset();
	}
	if (debugConsoleController) {
		debugConsoleController->removeCommands(*debugConsoleCommands);
		debugConsoleController.reset();
	}
	debugConsoleCommands.reset();
}

void ProjectWindow::reloadProject()
{
	destroy();
}

void ProjectWindow::addTask(std::unique_ptr<Task> task)
{
	tasks->addTask(std::move(task));
}

const ConfigNode& ProjectWindow::getSetting(EditorSettingType type, std::string_view id) const
{
	return settings.at(type)->getData(id);
}

void ProjectWindow::setSetting(EditorSettingType type, std::string_view id, ConfigNode data)
{
	settings.at(type)->setData(id, std::move(data));
}

const ConfigNode& ProjectWindow::getAssetSetting(std::string_view assetKey, std::string_view id)
{
	auto& data = settings.at(EditorSettingType::Project)->getMutableData(String("asset:") + assetKey);
	data.ensureType(ConfigNodeType::Map);
	return data[id];
}

void ProjectWindow::setAssetSetting(std::string_view assetKey, std::string_view id, ConfigNode value)
{
	auto& data = settings.at(EditorSettingType::Project)->getMutableData(String("asset:") + assetKey);
	data.ensureType(ConfigNodeType::Map);
	data[id] = std::move(value);
}

void ProjectWindow::openFileExternally(const Path& path)
{
	OS::get().openFile(path);
}

void ProjectWindow::showFileExternally(const Path& path)
{
	OS::get().showFile(path);
}

bool ProjectWindow::requestQuit(std::function<void()> callback)
{
	return assetEditorWindow->requestQuit(std::move(callback));
}

bool ProjectWindow::onQuitRequested()
{
	return requestQuit([=] ()
	{
		getAPI().core->quit();
	});
}

void ProjectWindow::closeProject()
{
	if (requestQuit([=] () { destroy(); })) {
		destroy();
	}
}

AssetPreviewGenerator& ProjectWindow::getAssetPreviewGenerator()
{
	if (!assetPreviewGenerator) {
		if (hasDLL) {
			project.withLoadedDLL([&] (ProjectDLL& dll)
			{
				assetPreviewGenerator = dll.getGame().createAssetPreviewGenerator(getAPI(), project.getGameResources());
				assetPreviewGenerator->setColourScheme(factory.getColourScheme());
			});
		} else {
			throw Exception("Unable to create Asset Preview Generator", HalleyExceptions::Core);
		}
	}
	return *assetPreviewGenerator;
}

Future<AssetPreviewData> ProjectWindow::getAssetPreviewData(AssetType assetType, const String& id, Vector2i size)
{
	auto cached = project.getCachedAssetPreview(assetType, id);
	if (cached) {
		// Convert image to sprite
		auto data = std::move(cached.value());
		data.sprite.setImage(project.getGameResources(), *api.video, data.image);
		return Future<AssetPreviewData>::makeImmediate(std::move(data));
	}

	return getAssetPreviewGenerator().getAssetPreviewData(assetType, id, size).then([=] (AssetPreviewData data) -> AssetPreviewData
	{
		// Store in cache
		project.setCachedAssetPreview(assetType, id, data);
		
		// Convert image to sprite
		data.sprite.setImage(project.getGameResources(), *api.video, data.image);
		return data;
	});
}

void ProjectWindow::render(RenderContext& rc) const
{
	if (assetPreviewGenerator) {
		assetPreviewGenerator->render(rc);
	}
}

Vector2f ProjectWindow::getChoosePrefabWindowSize() const
{
	return getSize() - Vector2f(900.0f, 350.0f);
}

EntityEditorFactoryRoot& ProjectWindow::getEntityEditorFactoryRoot()
{
	return *entityEditorFactoryRoot;
}

std::shared_ptr<ScriptNodeTypeCollection> ProjectWindow::getScriptNodeTypes()
{
	return project.getGameInstance()->createScriptNodeTypeCollection();
}

void ProjectWindow::buildGame()
{
	addTask(std::make_unique<BuildProjectTask>(project));
}

void ProjectWindow::updateEditor()
{
	if (requestQuit([=] () { doUpdateEditor(); })) {
		doUpdateEditor();
	}
}

void ProjectWindow::doUpdateEditor()
{
	editor.updateEditor();
}


ProjectWindow::SettingsStorage::SettingsStorage(std::shared_ptr<ISaveData> saveData, String path)
	: saveData(std::move(saveData))
	, path(std::move(path))
{
	load();
}

ProjectWindow::SettingsStorage::~SettingsStorage()
{
	save();
}

bool ProjectWindow::SettingsStorage::save() const
{
	if (dirty) {
		if (saveData) {
			saveData->setData(path, Serializer::toBytes(data));
		}
		dirty = false;
		return true;
	}
	return false;
}

void ProjectWindow::SettingsStorage::load()
{
	data.getRoot() = ConfigNode::MapType();

	if (saveData) {
		const auto bytes = saveData->getData(path);
		if (!bytes.empty()) {
			Deserializer::fromBytes(data, bytes);
		}
	}
}

void ProjectWindow::SettingsStorage::setData(std::string_view key, ConfigNode value)
{
	auto& val = data.getRoot()[key];
	if (val != value) {
		val = std::move(value);
		dirty = true;
	}
}

const ConfigNode& ProjectWindow::SettingsStorage::getData(std::string_view key) const
{
	return data.getRoot()[key];
}

ConfigNode& ProjectWindow::SettingsStorage::getMutableData(std::string_view key)
{
	dirty = true;
	return data.getRoot()[key];
}
