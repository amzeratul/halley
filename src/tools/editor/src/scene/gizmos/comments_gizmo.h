#pragma once
#include "halley/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	class ProjectComment;
	class ProjectComments;

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

	private:
		ProjectComments& comments;
		UIFactory& factory;
		ISceneEditorWindow& sceneEditorWindow;

		Vector<SceneEditorGizmoHandle> handles;
		uint64_t lastVersion = 0;
		Vector2f lastMousePos;
		bool forceHighlight = false;

		void addComment(Vector2f pos);
		void editComment(const UUID& uuid);

		SceneEditorGizmoHandle makeHandle(const UUID& uuid, Vector2f pos);
		void updateHandles();
		Vector2f getWorldOffset() const;
    };
}
