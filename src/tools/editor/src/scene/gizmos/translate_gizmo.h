#pragma once
#include "../scene_editor_gizmo.h"

namespace Halley {
	class TranslateGizmo final : public SceneEditorGizmo {
	public:
		TranslateGizmo();
		void update(Time time, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter) const override;

	private:
		bool visible = false;
		SceneEditorGizmoHandle handle;

		Circle getMainHandle() const;
		void updateEntityData(Vector2f pos);
	};
}
