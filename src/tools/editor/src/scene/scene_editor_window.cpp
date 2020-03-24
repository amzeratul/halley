#include "scene_editor_window.h"


#include "halley/tools/project/project.h"
#include "scene_editor_canvas.h"
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

	const auto& dll = project.getGameDLL();
	if (dll) {
		getWidgetAs<SceneEditorCanvas>("canvas")->setGameDLL(dll);
	}
}
