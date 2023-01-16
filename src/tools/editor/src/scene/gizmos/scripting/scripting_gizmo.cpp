#include "scripting_gizmo.h"

#include <components/scriptable_component.h>

using namespace Halley;

ScriptingGizmo::ScriptingGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes)
	: SceneEditorGizmo(snapRules)
	, sceneEditorWindow(sceneEditorWindow)
	, factory(factory)
	, scriptNodeTypes(scriptNodeTypes)
{
}

Vector<String> ScriptingGizmo::getHighlightedComponents() const
{
	return { "EmbeddedScript" };
}

bool ScriptingGizmo::allowEntitySpriteSelection() const
{
	return false;
}
