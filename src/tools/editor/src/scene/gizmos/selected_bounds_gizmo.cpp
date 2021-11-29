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

bool SelectedBoundsGizmo::shouldInclude(const Sprite& sprite, const ISceneEditor& sceneEditor) const
{
	if (!sprite.hasMaterial() || !sprite.isVisible()) {
		return false;
	}

	const auto scale = sprite.getScale().abs();
	if (scale.x < 0.0001f || scale.y < 0.0001f) {
		return false;
	}

	return sceneEditor.shouldDrawOutline(sprite);
}

void SelectedBoundsGizmo::draw(Painter& painter, const ISceneEditor& sceneEditor) const
{
	const float width = 2;
	const auto colour = Colour4f(1, 0, 1, 1);

	constexpr bool mergeOutlines = true;
	if (mergeOutlines) {
		drawMerged(painter, sceneEditor, width, colour);
	} else {
		drawSplit(painter, sceneEditor, width, colour);
	}
}

void SelectedBoundsGizmo::drawSplit(Painter& painter, const ISceneEditor& sceneEditor, float width, Colour4f colour) const
{
	for (auto& e: getEntities()) {
		painter.clear({}, {}, 0);
		drawStencilTree(painter, e, sceneEditor);
		drawOutlineTree(painter, e, sceneEditor, width, colour);
	}
}

void SelectedBoundsGizmo::drawMerged(Painter& painter, const ISceneEditor& sceneEditor, float width, Colour4f colour) const
{
	drawSplit(painter, sceneEditor, width, colour.multiplyAlpha(0.4f));

	painter.clear({}, {}, 0);
	for (auto& e: getEntities()) {
		drawStencilTree(painter, e, sceneEditor);
	}
	for (auto& e: getEntities()) {
		drawOutlineTree(painter, e, sceneEditor, width, colour);
	}
}

void SelectedBoundsGizmo::drawStencilTree(Painter& painter, EntityRef entity, const ISceneEditor& sceneEditor) const
{
	if (const auto* sprite = entity.tryGetComponent<SpriteComponent>()) {
		if (shouldInclude(sprite->sprite, sceneEditor)) {
			drawStencilSprite(painter, sprite->sprite);
		}
	}
	for (const auto c: entity.getChildren()) {
		drawStencilTree(painter, c, sceneEditor);
	}
}

void SelectedBoundsGizmo::drawOutlineTree(Painter& painter, EntityRef entity, const ISceneEditor& sceneEditor, float width, Colour4f colour) const
{
	if (const auto* sprite = entity.tryGetComponent<SpriteComponent>()) {
		if (shouldInclude(sprite->sprite, sceneEditor)) {
			drawOutlineSprite(painter, sprite->sprite, width, colour);
		}
	}
	for (const auto c: entity.getChildren()) {
		drawOutlineTree(painter, c, sceneEditor, width, colour);
	}
}

void SelectedBoundsGizmo::drawStencilSprite(Painter& painter, const Sprite& sprite) const
{
	Sprite s = sprite;
	s.setMaterial(stencilMaterial->clone());
	s.getMutableMaterial().set(0, sprite.getMaterial().getTexture(0));
	s.draw(painter);
}

void SelectedBoundsGizmo::drawOutlineSprite(Painter& painter, const Sprite& sprite, float width, Colour4f colour) const
{
	const auto tex0 = sprite.getMaterial().getTexture(0);

	const auto zoom = getZoom();
	const float outline = width / zoom;
	const float padding = std::ceil(outline);

	const auto texelSize = Vector2f(1.0f, 1.0f) / Vector2f(tex0->getSize());
	const auto ro = padding * texelSize;
	const auto texRect1 = Rect4f(-ro, Vector2f(1, 1) + ro);
	const auto newSize = sprite.getSize() + 2 * Vector2f(padding, padding);

	const auto texGrad0 = texelSize / zoom;
	const auto texGrad1 = texRect1.getSize() / newSize;
	const auto texGrads = Vector4f(texGrad0.x, texGrad0.y, texGrad1.x, texGrad1.y);

	Sprite s = sprite;
	s.setMaterial(outlineMaterial->clone());
	s.setColour(colour);
	s.getMutableMaterial().set(0, tex0);
	s.setCustom0(texGrads);
	s.setTexRect1(Rect4f(0, 0, 1, 1));
	s.crop(Vector4f(-padding, -padding, -padding, -padding));

	s.draw(painter);
}
