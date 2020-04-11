#include "selected_bounds_gizmo.h"
#include "halley/core/graphics/painter.h"
#include "scene_editor/scene_editor.h"
using namespace Halley;

void SelectedBoundsGizmo::update(Time time)
{}

void SelectedBoundsGizmo::draw(Painter& painter) const
{
	if (bounds) {
		painter.drawRect(bounds.value(), 1.0f / getZoom(), Colour4f(0.5f, 0.5f, 0.5f));
	}
}

void SelectedBoundsGizmo::onEntityChanged()
{
	const auto& e = getEntity();
	if (e) {
		bounds = SceneEditor::getSpriteTreeBounds(e.value());
	} else {
		bounds.reset();
	}
}
