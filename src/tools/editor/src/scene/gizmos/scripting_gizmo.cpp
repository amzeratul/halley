#include "scripting_gizmo.h"
using namespace Halley;

ScriptingGizmo::ScriptingGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow)
	: SceneEditorGizmo(snapRules)
	, factory(factory)
	, sceneEditorWindow(sceneEditorWindow)
{}

void ScriptingGizmo::update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState)
{
	// TODO
}

void ScriptingGizmo::draw(Painter& painter) const
{
	// TODO
}

bool ScriptingGizmo::isHighlighted() const
{
	// TODO
	return false;
}

std::shared_ptr<UIWidget> ScriptingGizmo::makeUI()
{
	// TODO
	return {};
}

std::vector<String> ScriptingGizmo::getHighlightedComponents() const
{
	return { "Script" };
}
