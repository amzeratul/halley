#pragma once
#include "halley/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	class CommentsGizmo final : public SceneEditorGizmo {
	public:
		explicit CommentsGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow);

		void update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter, const ISceneEditor& sceneEditor) const override;
		std::shared_ptr<UIWidget> makeUI() override;

		bool isHighlighted() const override;
		Vector<String> getHighlightedComponents() const override;
		bool onKeyPress(KeyboardKeyPress key) override;
		bool canBoxSelectEntities() const override;
    };
}
