#include "selection_box_gizmo.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/resources/resources.h"
using namespace Halley;

SelectionBoxGizmo::SelectionBoxGizmo(SnapRules snapRules, Resources& resources)
	: SceneEditorGizmo(snapRules)
{
}

void SelectionBoxGizmo::update(Time time, const SceneEditorInputState& inputState)
{
	bounds = inputState.selectionBox;
}

void SelectionBoxGizmo::draw(Painter& painter) const
{
	if (bounds) {
		const auto rect = bounds.value() + Vector2f(0.5f, 0.5f) / getZoom();
		painter.drawRect(rect, 3.0f / getZoom(), Colour4f(0, 0, 0, 0.5f));
		painter.drawRect(rect, 1.0f / getZoom(), Colour4f(1, 1, 1));
	}
}
