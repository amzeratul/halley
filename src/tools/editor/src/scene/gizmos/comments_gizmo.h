#pragma once
#include "halley/editor_extensions/scene_editor_gizmo.h"
#include "src/ui/popup_window.h"

namespace Halley {
	enum class ProjectCommentPriority;
	class ProjectComment;
	class ProjectComments;

	class CommentsGizmo final : public SceneEditorGizmo {
	public:
		explicit CommentsGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow);

		void update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter, const ISceneEditor& sceneEditor) const override;
		std::shared_ptr<UIWidget> makeUI() override;

		bool isHighlighted() const override;
		bool blockRightClick() const override;
		bool allowEntitySpriteSelection() const override;
		Vector<String> getHighlightedComponents() const override;
		bool onKeyPress(KeyboardKeyPress key) override;
		bool canBoxSelectEntities() const override;
		bool canSelectEntities() const override;

	private:
		ProjectComments& comments;
		UIFactory& factory;
		ISceneEditorWindow& sceneEditorWindow;

		Vector<SceneEditorGizmoHandle> handles;
		uint64_t lastVersion = std::numeric_limits<uint64_t>::max();
		Vector2f lastMousePos;
		bool forceHighlight = false;

		Sprite commentBg;
		Sprite commentOutline;
		Sprite commentIconNormal;
		Sprite tooltipBg;
		TextRenderer tooltipText;
		float nodeScale = 1.0f;

		void addComment(Vector2f pos, bool isWorldSpace);
		void editComment(const UUID& uuid, std::function<void(bool)> callback = {});
		void deleteComment(const UUID& uuid);
		void deleteComments();

		SceneEditorGizmoHandle makeHandle(const UUID& uuid, Vector2f pos);
		void updateHandles();
		Vector2f getWorldOffset() const;
		Colour4f getCommentColour(ProjectCommentPriority priority) const;
    };

	class CommentEditWindow : public PopupWindow {
	public:
		using Callback = std::function<void(bool)>;
		CommentEditWindow(UIFactory& factory, ProjectComments& comments, const UUID& uuid, Callback callback);

		void onAddedToRoot(UIRoot& root) override;
		void onRemovedFromRoot(UIRoot& root) override;

		void onMakeUI() override;
		bool onKeyPress(KeyboardKeyPress key) override;

	private:
		UIFactory& factory;
		ProjectComments& comments;
		UUID uuid;
		Callback callback;

		void loadComment();
		void onOK();
		void onCancel();
	};
}
