#include "scene_editor_window.h"



#include "entity_editor.h"
#include "entity_list.h"
#include "halley/tools/project/project.h"
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

		EntityFactory factory(world, project.getGameResources());
		
		sceneName = name;
		sceneId = factory.createEntity(name).getEntityId();
		interface.spawnPending();

		prefab = std::make_unique<Prefab>(*project.getGameResources().get<Prefab>(name));
		sceneData = std::make_unique<PrefabSceneData>(*prefab);
		entityEditor->setSceneData(*sceneData, project.getECSData());
		entityEditor->addFieldFactories(interface.getComponentEditorFieldFactories());

		entityList->clearExceptions();
		entityList->addException(interface.getCameraId());
		entityList->refreshList(world);
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
}

void SceneEditorWindow::update(Time t, bool moved)
{
	if (canvas->needsReload()) {
		String name = sceneName;
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
	entityEditor = getWidgetAs<EntityEditor>("entityEditor");

	setHandle(UIEventType::ListSelectionChanged, "entityList_list", [=](const UIEvent& event)
	{
		entityEditor->showEntity(event.getStringData());
	});
}

void SceneEditorWindow::load()
{
	const auto& dll = project.getGameDLL();
	if (dll) {
		canvas->loadGame(dll, project.getGameResources());
	}
}

