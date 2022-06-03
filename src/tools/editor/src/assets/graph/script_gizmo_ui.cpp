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

		if (inputState.selectionBox) {
			const float zoom = 1.0f;
			const auto rect = inputState.selectionBox.value() + Vector2f(0.5f, 0.5f) / zoom;
			p.drawRect(rect, 3.0f / zoom, Colour4f(0, 0, 0, 0.5f));
			p.drawRect(rect, 1.0f / zoom, Colour4f(1, 1, 1));
		}
	});
}

void ScriptGizmoUI::setZoom(float zoom)
{
	gizmo.setZoom(zoom);
}

bool ScriptGizmoUI::isHighlighted() const
{
	return gizmo.isHighlighted();
}

std::shared_ptr<UIWidget> ScriptGizmoUI::makeUI()
{
	return gizmo.makeUI();
}

void ScriptGizmoUI::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	inputState.mousePos = mousePos;
	updateSelectionBox();
	if (button == 0) {
		inputState.leftClickPressed = true;
		inputState.leftClickHeld = true;
		if (!isHighlighted()) {
			dragStart = mousePos;
		}
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
	updateSelectionBox();
	if (button == 0) {
		inputState.leftClickReleased = true;
		inputState.leftClickHeld = false;
		dragStart = {};
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
	updateSelectionBox();
}

bool ScriptGizmoUI::ignoreClip() const
{
	return true;
}

void ScriptGizmoUI::onModified()
{
	if (modifiedCallback) {
		modifiedCallback();
	}
}

void ScriptGizmoUI::updateSelectionBox()
{
	inputState.selectionBox.reset();
	if (dragStart && inputState.mousePos) {
		const auto rect = Rect4f(*dragStart, *inputState.mousePos);
		if (rect.getWidth() >= 2 || rect.getHeight() >= 2) {
			inputState.selectionBox = rect;
		}
	}
}
