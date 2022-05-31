#include "script_gizmo_ui.h"
using namespace Halley;

ScriptGizmoUI::ScriptGizmoUI(UIFactory& factory, Resources& resources, const IEntityEditorFactory& entityEditorFactory, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, ModifiedCallback modifiedCallback)
	: UIWidget("scriptGizmoUI", {}, {})
	, factory(factory)
	, resources(resources)
	, gizmo(factory, entityEditorFactory, nullptr, scriptNodeTypes)
	, modifiedCallback(std::move(modifiedCallback))
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
		inputState.rawMousePos = inputState.mousePos;
		gizmo.update(time, resources, inputState);
		inputState.clear();
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
	inputState.mousePos = mousePos;
	if (button == 0) {
		inputState.leftClickPressed = true;
		inputState.leftClickHeld = true;
	} else if (button == 1) {
		inputState.middleClickPressed = true;
		inputState.middleClickHeld = true;
	} else if (button == 2) {
		inputState.rightClickPressed = true;
		inputState.rightClickHeld = true;	
	}
}

void ScriptGizmoUI::releaseMouse(Vector2f mousePos, int button)
{
	inputState.mousePos = mousePos;
	if (button == 0) {
		inputState.leftClickReleased = true;
		inputState.leftClickHeld = false;
	} else if (button == 1) {
		inputState.middleClickReleased = true;
		inputState.middleClickHeld = false;
	} else if (button == 2) {
		inputState.rightClickReleased = true;
		inputState.rightClickHeld = false;
	}
}

void ScriptGizmoUI::onMouseOver(Vector2f mousePos)
{
	inputState.mousePos = mousePos;
}

void ScriptGizmoUI::onModified()
{
	if (modifiedCallback) {
		modifiedCallback();
	}
}
