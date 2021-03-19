#pragma once
#include "halley/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	class TranslateGizmo final : public SceneEditorGizmo {
	public:
		explicit TranslateGizmo(SnapRules snapRules);
		void update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter) const override;
		bool isHighlighted() const override;
		
	private:
		bool visible = false;
		SceneEditorGizmoHandle handle;
		Vector2f handleOffset;

		Circle getMainHandle() const;
		void updateEntityData(Vector2f pos);
		Vector2f getObjectOffset() const;
	};
}
