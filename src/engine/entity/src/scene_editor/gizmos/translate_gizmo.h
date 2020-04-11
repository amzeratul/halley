#pragma once
#include "scene_editor/scene_editor_gizmo.h"

namespace Halley {
	class TranslateGizmo final : public SceneEditorGizmo {
	public:
		void update(Time time, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter) const override;

	protected:
		void onEntityChanged() override;

	private:
		bool visible = false;
		bool hover = false;
		bool holding = false;

		Vector2f pos;
		Vector2f startOffset;
	};
}
