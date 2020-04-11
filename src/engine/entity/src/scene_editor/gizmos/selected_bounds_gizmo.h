#pragma once
#include "scene_editor/scene_editor_gizmo.h"

namespace Halley {
	class SelectedBoundsGizmo final : public SceneEditorGizmo {
	public:
		void update(Time time) override;
		void draw(Painter& painter) const override;

	protected:
		void onEntityChanged() override;

	private:
		std::optional<Rect4f> bounds;
	};
}
