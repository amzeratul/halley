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
	gizmos = std::make_unique<SceneEditorGizmoCollection>(factory, resources, sceneEditorWindow);

	gameResources = &project.getGameResources();
	project.withLoadedDLL([&] (ProjectDLL& dll)
	{
		load();
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

void SceneEditorGameBridge::update(Time t, SceneEditorInputState inputState, SceneEditorOutputState& outputState)
{
	if (errorState) {
		unload();
	}

	if (interface) {
		initializeInterfaceIfNeeded();
		if (interfaceReady) {
			interface->update(t, inputState, outputState);
		}
	}
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

void SceneEditorGameBridge::initializeInterfaceIfNeeded()
{	
	if (!interface) {
		project.withLoadedDLL([&] (ProjectDLL& dll)
		{
			load();
		});
	}

	if (interface && !interfaceReady) {
		if (interface->isReadyToCreateWorld()) {
			guardedRun([&]() {
				interface->createWorld(factory.getColourScheme());

				SceneEditorInputState inputState;
				SceneEditorOutputState outputState;
				interface->update(0, inputState, outputState);
				interfaceReady = true;
			}, true);
		}
	}
}

SceneEditorGizmoCollection& SceneEditorGameBridge::getGizmos() const
{
	return *gizmos;
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

void SceneEditorGameBridge::setSelectedEntity(const UUID& uuid, EntityData& data)
{
	if (interfaceReady) {
		interface->setSelectedEntity(uuid, data);
	}
}

void SceneEditorGameBridge::setEntityHighlightedOnList(const UUID& uuid)
{
	if (interfaceReady) {
		interface->setEntityHighlightedOnList(uuid);
	}
}

void SceneEditorGameBridge::showEntity(const UUID& uuid)
{
	if (interfaceReady) {
		interface->showEntity(uuid);
	}
}

void SceneEditorGameBridge::onEntityAdded(const UUID& uuid, const EntityData& data)
{
	if (interfaceReady) {
		interface->onEntityAdded(uuid, data);
	}
}

void SceneEditorGameBridge::onEntityRemoved(const UUID& uuid)
{
	if (interfaceReady) {
		interface->onEntityRemoved(uuid);
	}
}

void SceneEditorGameBridge::onEntityModified(const UUID& uuid,  const EntityData& oldData, const EntityData& newData)
{
	if (interfaceReady) {
		interface->onEntityModified(uuid, newData);
	}
	gizmos->onEntityModified(uuid, oldData, newData);
}

void SceneEditorGameBridge::onEntityMoved(const UUID& uuid, const EntityData& data)
{
	if (interfaceReady) {
		interface->onEntityMoved(uuid, data);
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

void SceneEditorGameBridge::setupConsoleCommands(UIDebugConsoleController& controller, ISceneEditorWindow& sceneEditor)
{
	if (interfaceReady) {
		interface->setupConsoleCommands(controller, sceneEditor);
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
	return Vector2f();
}

bool SceneEditorGameBridge::saveAsset(const Path& path, gsl::span<const gsl::byte> data)
{
	return project.writeAssetToDisk(path, data);
}

void SceneEditorGameBridge::addTask(std::unique_ptr<Task> task)
{
	projectWindow.addTask(std::move(task));
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

void SceneEditorGameBridge::refreshAssets()
{
	if (interfaceReady) {
		interface->refreshAssets();
	}
}

std::shared_ptr<ScriptNodeTypeCollection> SceneEditorGameBridge::getScriptNodeTypes()
{
	if (interfaceReady) {
		return interface->getScriptNodeTypes();
	}
	return {};
}

std::vector<std::pair<String, String>> SceneEditorGameBridge::getRightClickMenu(const Vector2f& mousePos) const
{
	if (interfaceReady) {
		return interface->getRightClickMenu(mousePos);
	}

	return std::vector<std::pair<String, String>>();
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
		context.resources = gameResources;
		context.editorResources = &resources;
		context.api = gameAPI.get();
		context.gizmos = gizmos.get();
		context.editorInterface = this;

		guardedRun([&]() {
			interface->init(context);
		});
		if (errorState) {
			unload();
		} else {
			initializeInterfaceIfNeeded();
		}
	}
}

void SceneEditorGameBridge::unload()
{
	interface.reset();
	interfaceReady = false;

	gameAPI.reset();
	gameCoreAPI.reset();

	errorState = false;

	gizmos->clear();
}

void SceneEditorGameBridge::guardedRun(const std::function<void()>& f, bool allowFailure) const
{
	try {
		f();
	} catch (const std::exception& e) {
		Logger::logException(e);
		if (!allowFailure) {
			errorState = true;
		}
	} catch (...) {
		Logger::logError("Unknown error in SceneEditorCanvas, probably from game dll");
		if (!allowFailure) {
			errorState = true;
		}
	}
}
