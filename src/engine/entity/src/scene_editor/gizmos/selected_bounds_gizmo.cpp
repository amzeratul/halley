#include "selected_bounds_gizmo.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/resources/resources.h"
#include "scene_editor/scene_editor.h"
using namespace Halley;

SelectedBoundsGizmo::SelectedBoundsGizmo(Resources& resources)
{
	material = std::make_shared<Material>(resources.get<MaterialDefinition>("Halley/InvertLine"));
}

void SelectedBoundsGizmo::update(Time time)
{}

void SelectedBoundsGizmo::draw(Painter& painter) const
{
	if (bounds) {
		painter.drawRect(bounds.value(), 1.0f / getZoom(), Colour4f(1, 1, 1), material);
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
