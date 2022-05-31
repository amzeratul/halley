#include "script_gizmo_ui.h"
using namespace Halley;

ScriptGizmoUI::ScriptGizmoUI(UIFactory& factory, Resources& resources, const IEntityEditorFactory& entityEditorFactory, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes)
	: UIWidget("scriptGizmoUI", {}, {})
	, factory(factory)
	, resources(resources)
	, gizmo(factory, entityEditorFactory, nullptr, scriptNodeTypes)
{
	gizmo.setModifiedCallback([=] ()
	{
		onModified();
	});
}

void ScriptGizmoUI::onAddedToRoot(UIRoot& root)
{
	gizmo.setUIRoot(root);
}

void ScriptGizmoUI::load(ScriptGraph& graph)
{
	gizmo.setGraph(&graph);
}

void ScriptGizmoUI::update(Time time, bool moved)
{
	gizmo.setBasePosition(getPosition());
	if (time > 0.00001) {
		gizmo.update(time, resources, inputState);
		inputState = SceneEditorInputState();
	}
}

void ScriptGizmoUI::draw(UIPainter& painter) const
{
	painter.draw([=] (Painter& p)
	{
		gizmo.draw(p);
	});
}

bool ScriptGizmoUI::isHighlighted() const
{
	return gizmo.isHighlighted();
}

void ScriptGizmoUI::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
}

void ScriptGizmoUI::releaseMouse(Vector2f mousePos, int button)
{
}

void ScriptGizmoUI::onMouseOver(Vector2f mousePos)
{
	inputState.mousePos = mousePos;
}

void ScriptGizmoUI::onModified()
{
}
