#pragma once
#include "halley/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	class ScriptingGizmo final : public SceneEditorGizmo {
	public:
		ScriptingGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes);
		
		void update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter) const override;
		bool isHighlighted() const override;
		std::shared_ptr<UIWidget> makeUI() override;
		std::vector<String> getHighlightedComponents() const override;
		void refreshEntity() override;
		void onEntityChanged() override;
	
	private:
		UIFactory& factory;
		ISceneEditorWindow& sceneEditorWindow;
		std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes;
		std::shared_ptr<ScriptRenderer> renderer;

		Vector2f basePos;
		ScriptGraph* scriptGraph = nullptr;
		std::optional<std::pair<uint32_t, Rect4f>> nodeUnderMouse;

		bool dragging = false;
		Vector2f startDragPos;

		mutable TextRenderer tooltipLabel;

		void loadEntityData();
		void saveEntityData();
		void drawToolTip(Painter& painter, const ScriptGraphNode& node, Rect4f nodePos) const;
	};
}
