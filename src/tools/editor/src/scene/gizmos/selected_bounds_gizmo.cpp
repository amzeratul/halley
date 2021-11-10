#include "selected_bounds_gizmo.h"

#include <components/sprite_component.h>

#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/resources/resources.h"
using namespace Halley;

SelectedBoundsGizmo::SelectedBoundsGizmo(SnapRules snapRules, Resources& resources)
	: SceneEditorGizmo(snapRules)
{
	stencilMaterial = std::make_shared<Material>(resources.get<MaterialDefinition>("Halley/SelectionStencil"));
	outlineMaterial = std::make_shared<Material>(resources.get<MaterialDefinition>("Halley/SelectionOutline"));
}

void SelectedBoundsGizmo::update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState)
{
}

void SelectedBoundsGizmo::draw(Painter& painter) const
{
	for (auto& e: getEntities()) {
		drawEntity(painter, e);
	}
}

void SelectedBoundsGizmo::drawEntity(Painter& painter, EntityRef entity) const
{
	painter.clear({}, {}, 0);
	drawStencilTree(painter, entity);
	drawOutlineTree(painter, entity);
}

void SelectedBoundsGizmo::drawStencilTree(Painter& painter, EntityRef entity) const
{
	if (const auto* sprite = entity.tryGetComponent<SpriteComponent>()) {
		drawStencilSprite(painter, sprite->sprite);
	}
	for (const auto c: entity.getChildren()) {
		drawStencilTree(painter, c);
	}
}

void SelectedBoundsGizmo::drawOutlineTree(Painter& painter, EntityRef entity) const
{
	if (const auto* sprite = entity.tryGetComponent<SpriteComponent>()) {
		drawOutlineSprite(painter, sprite->sprite);
	}
	for (const auto c: entity.getChildren()) {
		drawOutlineTree(painter, c);
	}
}

void SelectedBoundsGizmo::drawStencilSprite(Painter& painter, const Sprite& sprite) const
{
	Sprite s = sprite;
	s.setMaterial(stencilMaterial, false);
	s.getMutableMaterial().set(0, sprite.getMaterial().getTexture(0));
	s.draw(painter);
}

void SelectedBoundsGizmo::drawOutlineSprite(Painter& painter, const Sprite& sprite) const
{
	Sprite s = sprite;
	s.setMaterial(outlineMaterial, false);
	s.getMutableMaterial().set(0, sprite.getMaterial().getTexture(0));
	s.draw(painter);
}
