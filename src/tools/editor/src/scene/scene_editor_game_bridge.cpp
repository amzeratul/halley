#include "scene_editor_game_bridge.h"
#include "scene_editor_gizmo_collection.h"
#include "scene_editor_window.h"
#include "halley/tools/project/project.h"
#include "src/project/core_api_wrapper.h"
#include "src/ui/project_window.h"
using namespace Halley;


SceneEditorGameBridge::SceneEditorGameBridge(const HalleyAPI& api, Resources& resources, UIFactory& factory, Project& project, ProjectWindow& projectWindow, SceneEditorWindow& sceneEditorWindow)
	: api(api)
	, resources(resources)
	, project(project)
	, projectWindow(projectWindow)
	, factory(factory)
	, sceneEditorWindow(sceneEditorWindow)
{
	project.withLoadedDLL([&] (ProjectDLL& dll)
	{
		//load();
	});
}

SceneEditorGameBridge::~SceneEditorGameBridge()
{
	unload();
}

bool SceneEditorGameBridge::isLoaded() const
{
	return interfaceReady;
}

ISceneEditor& SceneEditorGameBridge::getInterface() const
{
	Expects(interface);
	return *interface;
}

bool SceneEditorGameBridge::update(Time t, SceneEditorInputState inputState, SceneEditorOutputState& outputState)
{
	if (errorState) {
		unload();
	}

	if (interface) {
		initializeInterfaceIfNeeded(false);
		if (interfaceReady && t > 0.0001) {
			interface->update(t, inputState, outputState);
			return true;
		}
	}
	return false;
}

void SceneEditorGameBridge::render(RenderContext& rc) const
{
	if (errorState) {
		return;
	}

	if (interface && interfaceReady) {
		guardedRun([&]() {
			interface->render(rc);
		});
	}
}

void SceneEditorGameBridge::initializeInterfaceIfNeeded(bool force)
{	
	if (!interface) {
		interfaceInitializationError = false;
		project.withLoadedDLL([&] (ProjectDLL& dll)
		{
			load();
		});
	}

	if (interface && !interfaceReady) {
		if (force || !interfaceInitializationError) {
			if (interface->isReadyToCreateWorld()) {
				const bool success = guardedRun([&]() {
					interface->setGameInstance(project.getGameInstance());
					interface->createWorld(*prefab, factory.getColourScheme());
					interface->setAssetPreviewGenerator(projectWindow.getAssetPreviewGenerator());
					interface->setGameInstance(project.getGameInstance());

					SceneEditorInputState inputState;
					SceneEditorOutputState outputState;
					interface->update(0, inputState, outputState);
					interfaceReady = true;
				}, true);

				interfaceInitializationError = !success;
			}
		}
	}
}

SceneEditorGizmoCollection& SceneEditorGameBridge::getGizmos()
{
	if (!gizmos) {
		gizmos = std::make_unique<SceneEditorGizmoCollection>(factory, resources, sceneEditorWindow);
	}
	return *gizmos;
}

void SceneEditorGameBridge::adjustView(int zoomChange, bool zoomToFit, bool centre)
{
	if (interfaceReady) {
		interface->adjustView(zoomChange, zoomToFit, centre);
	}
}

void SceneEditorGameBridge::changeZoom(int amount, Vector2f mousePos)
{
	if (interfaceReady) {
		interface->changeZoom(amount, mousePos);
	}
}

void SceneEditorGameBridge::dragCamera(Vector2f pos)
{
	if (interfaceReady) {
		interface->dragCamera(pos);
	}
}

void SceneEditorGameBridge::moveCamera(Vector2f pos)
{
	if (interfaceReady) {
		interface->moveCamera(pos);
	}
}

bool SceneEditorGameBridge::loadCameraPos()
{
	if (interfaceReady) {
		return interface->loadCameraPos();
	}
	return false;
}

std::shared_ptr<UIWidget> SceneEditorGameBridge::makeCustomUI() const
{
	std::shared_ptr<UIWidget> result;
	if (interfaceReady) {
		guardedRun([&] ()
		{
			result = interface->makeCustomUI();
		});
	}
	return result;
}

void SceneEditorGameBridge::setSelectedEntities(Vector<UUID> uuids, Vector<EntityData*> datas)
{
	if (interfaceReady) {
		interface->setSelectedEntities(std::move(uuids), std::move(datas));
	}
}

void SceneEditorGameBridge::setEntityHighlightedOnList(const UUID& uuid, bool forceShow)
{
	if (interfaceReady) {
		interface->setEntityHighlightedOnList(uuid, forceShow);
	}
}

void SceneEditorGameBridge::showEntity(const UUID& uuid)
{
	if (interfaceReady) {
		interface->showEntity(uuid);
	}
}

void SceneEditorGameBridge::onToolSet(String& tool, String& componentName, String& fieldName)
{
	if (interfaceReady) {
		interface->onToolSet(tool, componentName, fieldName);
	}
}

void SceneEditorGameBridge::onSceneLoaded(Prefab& scene)
{
	if (interfaceReady) {
		interface->onSceneLoaded(scene);
	}
}

void SceneEditorGameBridge::onSceneSaved()
{
	if (interfaceReady) {
		interface->onSceneSaved();
	}
}

void SceneEditorGameBridge::setupConsoleCommands(UIDebugConsoleCommands& commands, ISceneEditorWindow& sceneEditor)
{
	if (interfaceReady) {
		interface->setupConsoleCommands(commands, sceneEditor);
	}
}

void SceneEditorGameBridge::cycleHighlight(int delta)
{
	if (interfaceReady) {
		interface->cycleHighlight(delta);
	}
}

std::optional<Vector2f> SceneEditorGameBridge::getMousePos() const
{
	if (interfaceReady) {
		return interface->getMousePos();
	}
	return {};
}

Vector2f SceneEditorGameBridge::getCameraPos() const
{
	if (interfaceReady) {
		return interface->getCameraPos();
	}
	return {};
}

std::optional<Vector2f> SceneEditorGameBridge::getWorldOffset() const
{
	if (interfaceReady) {
		return interface->getWorldOffset();
	}
	return {};
}

bool SceneEditorGameBridge::saveAsset(const Path& path, gsl::span<const gsl::byte> data)
{
	return project.writeAssetToDisk(path, data);
}

void SceneEditorGameBridge::openAsset(AssetType assetType, const String& assetId)
{
	sceneEditorWindow.openAsset(assetType, assetId);
}

void SceneEditorGameBridge::openAssetHere(AssetType assetType, const String& assetId)
{
	sceneEditorWindow.openAssetHere(assetType, assetId);
}

void SceneEditorGameBridge::addTask(std::unique_ptr<Task> task)
{
	projectWindow.addTask(std::move(task));
}

void SceneEditorGameBridge::setAssetSaveNotification(bool enabled)
{
	project.setAssetSaveNotification(enabled);
}

const ConfigNode& SceneEditorGameBridge::getSetting(EditorSettingType type, std::string_view id) const
{
	return projectWindow.getSetting(type, id);
}

void SceneEditorGameBridge::setSetting(EditorSettingType type, std::string_view id, ConfigNode data)
{
	projectWindow.setSetting(type, id, std::move(data));
}

const ConfigNode& SceneEditorGameBridge::getAssetSetting(std::string_view id) const
{
	return projectWindow.getAssetSetting(sceneEditorWindow.getAssetKey(), id);
}

void SceneEditorGameBridge::setAssetSetting(std::string_view id, ConfigNode data)
{
	projectWindow.setAssetSetting(sceneEditorWindow.getAssetKey(), id, std::move(data));
}

const ConfigNode& SceneEditorGameBridge::getAssetSetting(std::string_view assetKey, std::string_view id) const
{
	return projectWindow.getAssetSetting(assetKey, id);
}

void SceneEditorGameBridge::setAssetSetting(std::string_view assetKey, std::string_view id, ConfigNode data)
{
	projectWindow.setAssetSetting(assetKey, id, std::move(data));
}

String SceneEditorGameBridge::getAssetKey()
{
	return sceneEditorWindow.getAssetKey();
}

void SceneEditorGameBridge::selectEntity(const String& uuid)
{
	sceneEditorWindow.selectEntity(uuid);
}

Sprite SceneEditorGameBridge::getEntityIcon(const String& uuid)
{
	const auto& entityData = sceneEditorWindow.getSceneData()->getEntityNodeData(uuid).getData();
	String icon;
	if (!entityData.getIcon().isEmpty()) {
		icon = entityData.getIcon();
	} else if (!entityData.getPrefab().isEmpty()) {
		auto& gameResources = project.getGameResources();
		if (gameResources.exists<Prefab>(entityData.getPrefab())) {
			const auto prefab = gameResources.get<Prefab>(entityData.getPrefab());
			icon = prefab->getPrefabIcon();
		}
	}
	return sceneEditorWindow.getEntityIcons().getIcon(icon);
}

Sprite SceneEditorGameBridge::getAssetIcon(AssetType type)
{
	return factory.makeAssetTypeIcon(type);
}

void SceneEditorGameBridge::clearAssetCache()
{
	project.clearCachedAssetPreviews();
}

void SceneEditorGameBridge::refreshAssets()
{
	if (interfaceReady) {
		interface->refreshAssets();
	}
}

Vector<UIPopupMenuItem> SceneEditorGameBridge::getSceneContextMenu(const Vector2f& mousePos) const
{
	if (interfaceReady) {
		return interface->getSceneContextMenu(mousePos);
	}

	return {};
}

void SceneEditorGameBridge::onSceneContextMenuSelection(const String& id)
{
	if (interfaceReady) {
		interface->onSceneContextMenuSelection(id);
	}
}

void SceneEditorGameBridge::onSceneContextMenuHighlight(const String& id)
{
	if (interfaceReady) {
		interface->onSceneContextMenuHighlight(id);
	}
}

Transform2DComponent* SceneEditorGameBridge::getTransform(const String& entityId)
{
	if (interfaceReady) {
		return interface->getTransform(entityId);
	}
	return nullptr;
}

void SceneEditorGameBridge::initializeEntityValidator(EntityValidator& validator)
{
	Expects(interfaceReady);
	interface->initializeEntityValidator(validator);
}

void SceneEditorGameBridge::validateAllEntities()
{
	sceneEditorWindow.validateAllEntities();
}

World& SceneEditorGameBridge::getWorld() const
{
	return interface->getWorld();
}

String SceneEditorGameBridge::getSceneNameForComments(AssetType assetType, const String& assetId) const
{
	if (interface) {
		return interface->getSceneNameForComments(assetType, assetId);
	}
	return assetId;
}

UIFactory& SceneEditorGameBridge::getFactory() const
{
	return factory;
}

void SceneEditorGameBridge::setPrefab(std::shared_ptr<Prefab> prefab)
{
	this->prefab = std::move(prefab);
}

void SceneEditorGameBridge::load()
{
	guardedRun([&]() {
		const auto game = project.getGameInstance();
		if (!game) {
			throw Exception("Unable to load scene editor", HalleyExceptions::Tools);
		}

		interface = game->createSceneEditorInterface();
		interfaceReady = false;
		errorState = false;
	});

	if (interface) {
		gameCoreAPI = std::make_unique<CoreAPIWrapper>(*api.core);
		gameAPI = api.clone();
		gameAPI->replaceCoreAPI(gameCoreAPI.get());

		SceneEditorContext context;
		context.editorResources = &resources;
		context.resources = &project.getGameResources();
		context.api = gameAPI.get();
		context.gizmos = &getGizmos();
		context.editorInterface = this;
		context.gameEditorData = project.getGameEditorData();

		guardedRun([&]() {
			interface->init(context);
		});
		if (errorState) {
			unload();
		} else {
			initializeInterfaceIfNeeded(true);
		}
	}
}

void SceneEditorGameBridge::unload()
{
	if (gizmos) {
		gizmos->clear();
		gizmos = {};
	}

	interface.reset();
	interfaceReady = false;

	gameAPI.reset();
	gameCoreAPI.reset();

	errorState = false;
}

void SceneEditorGameBridge::reloadScene()
{
	if (gizmos) {
		gizmos->clear();
	}
	getGizmos();
}

bool SceneEditorGameBridge::guardedRun(const std::function<void()>& f, bool allowFailure) const
{
	try {
		f();
		return true;
	} catch (const std::exception& e) {
		Logger::logException(e);
		if (!allowFailure) {
			errorState = true;
		}
		return false;
	} catch (...) {
		Logger::logError("Unknown error in SceneEditorCanvas, probably from game dll");
		if (!allowFailure) {
			errorState = true;
		}
		return false;
	}
}
