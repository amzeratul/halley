#pragma once
#include "halley/core/graphics/sprite/animation.h"
#include "scene_editor/scene_editor_gizmo.h"

namespace Halley {
	class Material;
	
	class SelectedBoundsGizmo final : public SceneEditorGizmo {
	public:
		explicit SelectedBoundsGizmo(Resources& resources);
		
		void draw(Painter& painter) const override;

	protected:
		void onEntityChanged() override;

	private:
		std::optional<Rect4f> bounds;
		std::shared_ptr<Material> material;
	};
}
