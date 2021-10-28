#include "selected_bounds_gizmo.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/resources/resources.h"
using namespace Halley;

SelectedBoundsGizmo::SelectedBoundsGizmo(SnapRules snapRules, Resources& resources)
	: SceneEditorGizmo(snapRules)
{
	material = std::make_shared<Material>(resources.get<MaterialDefinition>("Halley/InvertLine"));
}

void SelectedBoundsGizmo::update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState)
{
	bounds.clear();
	for (auto& e: getEntities()) {
		bounds.push_back(sceneEditor.getSpriteTreeBounds(e));
	}
}

void SelectedBoundsGizmo::draw(Painter& painter) const
{
	for (auto& b: bounds) {
		painter.drawRect(b, 1.0f / getZoom(), Colour4f(1, 1, 1), material);
	}
}
