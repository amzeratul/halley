#include "scene_editor_window.h"



#include "entity_editor.h"
#include "entity_list.h"
#include "halley/tools/file/filesystem.h"
#include "halley/tools/project/project.h"
#include "halley/tools/yaml/yaml_convert.h"
#include "scene_editor_canvas.h"
using namespace Halley;

SceneEditorWindow::SceneEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api)
	: UIWidget("scene_editor", {}, UISizer())
	, uiFactory(factory)
	, project(project)
{
	makeUI();
	load();
}

SceneEditorWindow::~SceneEditorWindow()
{
}

void SceneEditorWindow::loadScene(const String& name)
{
	Expects(canvas);
	
	unloadScene();
	
	if (!name.isEmpty() && canvas->isLoaded()) {
		auto& interface = canvas->getInterface();
		auto& world = interface.getWorld();

		// Load prefab
		prefab = std::make_unique<Prefab>(*project.getGameResources().get<Prefab>(name));

		// Spawn scene
		entityFactory = std::make_shared<EntityFactory>(world, project.getGameResources());
		auto entity = entityFactory->createEntity(prefab->getRoot());
		interface.spawnPending();

		// Get scene data
		sceneId = entity.getEntityId();
		sceneName = name;

		// Setup editors
		sceneData = std::make_shared<PrefabSceneData>(*prefab, entityFactory, world);
		entityEditor->setECSData(project.getECSData());
		entityEditor->addFieldFactories(interface.getComponentEditorFieldFactories());
		entityList->setSceneData(sceneData);

		// Show root
		panCameraToEntity(entity.getUUID().toString());
	}
}

void SceneEditorWindow::unloadScene()
{
	Expects(canvas);
	
	if (canvas->isLoaded() && sceneId.isValid()) {
		auto& world = canvas->getInterface().getWorld();
		world.destroyEntity(sceneId);
		world.spawnPending();
	}
	sceneName = "";
	sceneId = EntityId();
	entityFactory.reset();
	sceneData.reset();
}

void SceneEditorWindow::update(Time t, bool moved)
{
	if (canvas->needsReload()) {
		const String name = sceneName;
		unloadScene();
		canvas->reload();
		loadScene(name);
	}
}

void SceneEditorWindow::makeUI()
{
	add(uiFactory.makeUI("ui/halley/scene_editor_window"), 1);
	
	canvas = getWidgetAs<SceneEditorCanvas>("canvas");
	
	entityList = getWidgetAs<EntityList>("entityList");
	entityList->setSceneEditorWindow(*this);

	entityEditor = getWidgetAs<EntityEditor>("entityEditor");
	entityEditor->setSceneEditorWindow(*this);

	getWidget("saveButton")->setEnabled(false);

	setHandle(UIEventType::ListSelectionChanged, "entityList_list", [=] (const UIEvent& event)
	{
		selectEntity(event.getStringData());
	});

	setHandle(UIEventType::ListAccept, "entityList_list", [=](const UIEvent& event)
	{
		panCameraToEntity(event.getStringData());
	});

	setHandle(UIEventType::ButtonClicked, "saveButton", [=] (const UIEvent& event)
	{
		saveEntity();
	});
}

void SceneEditorWindow::load()
{
	const auto& dll = project.getGameDLL();
	if (dll) {
		canvas->loadGame(dll, project.getGameResources());
	}
}

void SceneEditorWindow::selectEntity(const String& id)
{
	const bool changed = entityEditor->loadEntity(id, sceneData->getEntityData(id), false);
	if (changed) {
		getWidget("saveButton")->setEnabled(false);
		currentEntityId = id;
	}
}

void SceneEditorWindow::panCameraToEntity(const String& id)
{
	canvas->getInterface().showEntity(UUID(id));
}

void SceneEditorWindow::saveEntity()
{
	getWidget("saveButton")->setEnabled(false);

	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = {{ "name", "uuid", "components", "children" }};

	auto path = project.getAssetsSrcPath() / "prefab" / (sceneName + ".yaml");
	auto strData = YAMLConvert::generateYAML(*prefab, options);
	auto data = gsl::as_bytes(gsl::span<const char>(strData.c_str(), strData.length()));
	FileSystem::writeFile(path, data);
}

void SceneEditorWindow::markModified()
{
	getWidget("saveButton")->setEnabled(true);
}

void SceneEditorWindow::onTreeChanged()
{
	entityEditor->loadEntity(currentEntityId, sceneData->getEntityData(currentEntityId), true);
	markModified();
}

void SceneEditorWindow::onEntityModified(const String& id)
{
	entityList->onEntityModified(id, sceneData->getEntityData(id));
	sceneData->reloadEntity(id);
	markModified();
}

