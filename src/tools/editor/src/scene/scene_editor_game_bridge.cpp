#include "scene_editor_game_bridge.h"
#include "scene_editor_gizmo_collection.h"
#include "halley/tools/project/project.h"
#include "src/project/core_api_wrapper.h"
using namespace Halley;


SceneEditorGameBridge::SceneEditorGameBridge(const HalleyAPI& api, Resources& resources, UIFactory& factory, Project& project)
	: api(api)
	, resources(resources)
{
	gizmos = std::make_unique<SceneEditorGizmoCollection>(factory, resources);

	gameResources = &project.getGameResources();
	gameDLL = project.getGameDLL();
	if (gameDLL) {
		loadDLL();
	}
}

SceneEditorGameBridge::~SceneEditorGameBridge()
{
	unloadDLL();
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
		unloadDLL();
	}

	if (interface) {
		initializeInterfaceIfNeeded();
		interface->update(t, inputState, outputState);
	}
}

void SceneEditorGameBridge::render(RenderContext& rc) const
{
	if (errorState) {
		return;
	}

	if (interface) {
		guardedRun([&]() {
			interface->render(rc);
		});
	}
}

void SceneEditorGameBridge::initializeInterfaceIfNeeded()
{
	if (!interfaceReady) {
		if (interface->isReadyToCreateWorld()) {
			guardedRun([&]() {
				interface->createWorld();
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

void SceneEditorGameBridge::setSelectedEntity(const UUID& uuid, ConfigNode& data)
{
	if (interfaceReady) {
		interface->setSelectedEntity(uuid, data);
	}
}

void SceneEditorGameBridge::showEntity(const UUID& uuid)
{
	if (interfaceReady) {
		interface->showEntity(uuid);
	}
}

void SceneEditorGameBridge::onEntityAdded(const UUID& uuid, const ConfigNode& data)
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

void SceneEditorGameBridge::onEntityModified(const UUID& uuid, const ConfigNode& data)
{
	if (interfaceReady) {
		interface->onEntityModified(uuid, data);
	}
}

void SceneEditorGameBridge::onEntityMoved(const UUID& uuid, const ConfigNode& data)
{
	if (interfaceReady) {
		interface->onEntityMoved(uuid, data);
	}
}

ConfigNode SceneEditorGameBridge::onToolSet(SceneEditorTool tool, const String& componentName, const String& fieldName, ConfigNode options)
{
	if (interfaceReady) {
		return interface->onToolSet(tool, componentName, fieldName, std::move(options));
	}
	return options;
}

void SceneEditorGameBridge::loadDLL()
{
	Expects(gameDLL);

	gameDLL->load(true);
	const auto getHalleyEntry = static_cast<IHalleyEntryPoint * (HALLEY_STDCALL*)()>(gameDLL->getFunction("getHalleyEntry"));
	auto game = getHalleyEntry()->createGame();
	guardedRun([&]() {
		interface = game->createSceneEditorInterface();
		interfaceReady = false;
		errorState = false;
	});
	game.reset();

	if (interface) {
		gameCoreAPI = std::make_unique<CoreAPIWrapper>(*api.core);
		gameAPI = api.clone();
		gameAPI->replaceCoreAPI(gameCoreAPI.get());

		SceneEditorContext context;
		context.resources = gameResources;
		context.editorResources = &resources;
		context.api = gameAPI.get();
		context.gizmos = gizmos.get();

		guardedRun([&]() {
			interface->init(context);
		});
		if (errorState) {
			unloadDLL();
		} else {
			initializeInterfaceIfNeeded();
		}
	}
}

void SceneEditorGameBridge::unloadDLL()
{
	interface.reset();
	interfaceReady = false;

	if (gameDLL) {
		gameDLL->unload();
	}

	gameAPI.reset();
	gameCoreAPI.reset();

	errorState = false;
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
