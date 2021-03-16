#pragma once
#include "halley/core/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	class VertexGizmo final : public SceneEditorGizmo {
	public:
		VertexGizmo(SnapRules snapRules, String componentName, String fieldName);
		void update(Time time, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter) const override;

	private:
		String componentName;
		String fieldName;
		bool visible = false;
		SceneEditorGizmoHandle handle;

		Circle getMainHandle() const;
		void updateEntityData(Vector2f pos);
		Vector2f readEntityData() const;
	};
}
