#pragma once
#include "halley/core/graphics/sprite/animation.h"
#include "../scene_editor_gizmo.h"

namespace Halley {
	class Material;
	
	class SelectionBoxGizmo final : public SceneEditorGizmo {
	public:
		explicit SelectionBoxGizmo(Resources& resources);

		void update(Time time, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter) const override;

	private:
		std::optional<Rect4f> bounds;
	};
}
