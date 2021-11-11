#pragma once
#include "halley/core/graphics/sprite/animation.h"
#include "halley/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	class Material;
	
	class SelectedBoundsGizmo final : public SceneEditorGizmo {
	public:
		SelectedBoundsGizmo(SnapRules snapRules, Resources& resources);

		void update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter, const ISceneEditor& sceneEditor) const override;

	private:
		std::shared_ptr<Material> stencilMaterial;
		std::shared_ptr<Material> outlineMaterial;

		bool shouldInclude(const Sprite& sprite, const ISceneEditor& sceneEditor) const;
		void drawEntity(Painter& painter, EntityRef entity, const ISceneEditor& sceneEditor) const;
		void drawStencilTree(Painter& painter, EntityRef entity, const ISceneEditor& sceneEditor) const;
		void drawOutlineTree(Painter& painter, EntityRef entity, const ISceneEditor& sceneEditor) const;
		void drawStencilSprite(Painter& painter, const Sprite& sprite) const;
		void drawOutlineSprite(Painter& painter, const Sprite& sprite) const;
	};
}
