#include "scene_editor_window.h"
using namespace Halley;

SceneEditorWindow::SceneEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api)
	: UIWidget("assets_editor", {}, UISizer())
	, factory(factory)
	, project(project)
{
	makeUI();
}

void SceneEditorWindow::makeUI()
{
	add(factory.makeUI("ui/halley/scene_editor_window"), 1);
}
