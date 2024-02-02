#include "selection_box_gizmo.h"
#include "halley/graphics/painter.h"
#include "halley/resources/resources.h"
using namespace Halley;

SelectionBoxGizmo::SelectionBoxGizmo(SnapRules snapRules, Resources& resources)
	: SceneEditorGizmo(snapRules)
{
}

void SelectionBoxGizmo::update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState)
{
	bounds = inputState.selectionBox;
}

void SelectionBoxGizmo::draw(Painter& painter, const ISceneEditor& sceneEditor) const
{
	if (bounds) {
		const auto rect = bounds.value();
		painter.drawRect(rect, 3.0f / getZoom(), Colour4f(0, 0, 0, 0.5f));
		painter.drawRect(rect, 1.0f / getZoom(), Colour4f(1, 1, 1));
	}
}
