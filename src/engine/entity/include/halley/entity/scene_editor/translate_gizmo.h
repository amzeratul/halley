#pragma once
#include "scene_editor_gizmo.h"

namespace Halley {
	class Transform2DComponent;
	
	class TranslateGizmo final : public SceneEditorGizmo {
	public:
		void update(Time time) override;
		void draw(Painter& painter) const override;

	protected:
		void onEntityChanged() override;

	private:
		Vector2f pos;
		bool visible = false;
	};
}
