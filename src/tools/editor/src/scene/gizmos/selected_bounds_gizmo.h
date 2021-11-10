#pragma once
#include "halley/core/graphics/sprite/animation.h"
#include "halley/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	class Material;
	
	class SelectedBoundsGizmo final : public SceneEditorGizmo {
	public:
		SelectedBoundsGizmo(SnapRules snapRules, Resources& resources);

		void update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter) const override;

	private:
		std::shared_ptr<Material> stencilMaterial;
		std::shared_ptr<Material> outlineMaterial;

		void drawEntity(Painter& painter, EntityRef entity) const;
		void drawStencilTree(Painter& painter, EntityRef entity) const;
		void drawOutlineTree(Painter& painter, EntityRef entity) const;
		void drawStencilSprite(Painter& painter, const Sprite& sprite) const;
		void drawOutlineSprite(Painter& painter, const Sprite& sprite) const;
	};
}
