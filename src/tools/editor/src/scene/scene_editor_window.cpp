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
		sceneName = name;

		prefab = std::make_unique<Prefab>(*project.getGameResources().get<Prefab>(name));
		entityFactory = std::make_shared<EntityFactory>(world, project.getGameResources());

		auto entity = entityFactory->createEntity(prefab->getRoot());
		sceneId = entity.getEntityId();
		interface.spawnPending();

		sceneData = std::make_unique<PrefabSceneData>(*prefab, entityFactory, entity);
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
	entityEditor = getWidgetAs<EntityEditor>("entityEditor");
	entityEditor->setSceneEditor(*this);
	getWidget("saveButton")->setEnabled(false);

	setHandle(UIEventType::ListSelectionChanged, "entityList_list", [=] (const UIEvent& event)
	{
		showEntity(event.getStringData());
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

void SceneEditorWindow::showEntity(const String& id)
{
	getWidget("saveButton")->setEnabled(false);
	entityEditor->showEntity(id);
}

void SceneEditorWindow::saveEntity()
{
	getWidget("saveButton")->setEnabled(false);

	sceneName;
	auto& data = prefab->getRoot();
}

void SceneEditorWindow::markModified()
{
	getWidget("saveButton")->setEnabled(true);
}

