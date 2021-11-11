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

bool SelectedBoundsGizmo::shouldInclude(const Sprite& sprite) const
{
	if (!sprite.hasMaterial() || !sprite.isVisible()) {
		return false;
	}

	// TODO: let game decide
	const auto mask = sprite.getMaterial().getDefinition().getDefaultMask();
	return mask == 1 || mask == 2 || mask == 4;
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
		if (shouldInclude(sprite->sprite)) {
			drawStencilSprite(painter, sprite->sprite);
		}
	}
	for (const auto c: entity.getChildren()) {
		drawStencilTree(painter, c);
	}
}

void SelectedBoundsGizmo::drawOutlineTree(Painter& painter, EntityRef entity) const
{
	if (const auto* sprite = entity.tryGetComponent<SpriteComponent>()) {
		if (shouldInclude(sprite->sprite)) {
			drawOutlineSprite(painter, sprite->sprite);
		}
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
	const auto tex0 = sprite.getMaterial().getTexture(0);

	const auto zoom = std::max(1.0f, getZoom());
	const float outline = 2 / zoom;
	const float padding = std::ceil(outline);

	const auto texelSize = Vector2f(1.0f, 1.0f) / Vector2f(tex0->getSize());
	const auto ro = padding * texelSize;
	const auto texRect1 = Rect4f(-ro, Vector2f(1, 1) + ro);
	const auto newSize = sprite.getSize() + 2 * Vector2f(padding, padding);

	const auto texGrad0 = texelSize / zoom;
	const auto texGrad1 = texRect1.getSize() / newSize;
	const auto texGrads = Vector4f(texGrad0.x, texGrad0.y, texGrad1.x, texGrad1.y);

	Sprite s = sprite;
	s.setMaterial(outlineMaterial, false);
	s.setColour(Colour4f(1, 0, 1, 1));
	s.getMutableMaterial().set(0, tex0);
	s.setCustom0(texGrads);
	s.setTexRect1(Rect4f(0, 0, 1, 1));
	s.crop(Vector4f(-padding, -padding, -padding, -padding));

	s.draw(painter);
}
