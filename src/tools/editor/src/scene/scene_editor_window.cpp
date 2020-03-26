#include "scene_editor_window.h"


#include "halley/tools/project/project.h"
#include "scene_editor_canvas.h"
using namespace Halley;

SceneEditorWindow::SceneEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api)
	: UIWidget("assets_editor", {}, UISizer())
	, uiFactory(factory)
	, project(project)
{
	makeUI();
	load();
}

SceneEditorWindow::~SceneEditorWindow()
{
	unloadScene();
}

void SceneEditorWindow::loadScene(const String& name)
{
	Expects(canvas);
	
	unloadScene();
	
	if (!name.isEmpty() && canvas->isLoaded()) {
		auto& world = canvas->getInterface().getWorld();

		EntityFactory factory(world, project.getGameResources());
		
		sceneName = name;
		sceneId = factory.createEntity(name).getEntityId();
	}
}

void SceneEditorWindow::unloadScene()
{
	Expects(canvas);
	
	if (canvas->isLoaded() && sceneId.isValid()) {
		auto& world = canvas->getInterface().getWorld();
		world.destroyEntity(sceneId);
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

	setHandle(UIEventType::ButtonClicked, "load", [=](const UIEvent& event)
	{
		loadScene(getWidgetAs<UITextInput>("fileName")->getText());
	});
}

void SceneEditorWindow::load()
{
	const auto& dll = project.getGameDLL();
	if (dll) {
		canvas->loadGame(dll, project.getGameResources());
	}
}
