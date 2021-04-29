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

		void saveEntityData();
		void destroyNode(uint32_t id);
		ScriptGraphNode& getNode(uint32_t id);

	private:
		UIFactory& factory;
		ISceneEditorWindow& sceneEditorWindow;
		std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes;
		std::shared_ptr<ScriptRenderer> renderer;

		Vector2f basePos;
		ScriptGraph* scriptGraph = nullptr;
		std::optional<ScriptRenderer::NodeUnderMouseInfo> nodeUnderMouse;
		std::optional<ScriptRenderer::NodeUnderMouseInfo> nodeEditingConnection;
		std::optional<Vector2f> nodeConnectionDst;

		bool dragging = false;
		Vector2f startDragPos;

		mutable TextRenderer tooltipLabel;

		void loadEntityData();
		void drawToolTip(Painter& painter, const ScriptGraphNode& node, Rect4f nodePos) const;

		void openNodeUI(uint32_t nodeId, Vector2f pos);
		void addNode();
		void addNode(const String& type);

		void onNodeClicked(Vector2f mousePos);
		void onNodeDragging(const SceneEditorInputState& inputState);
		void onPinClicked();
		void onEditingConnection(const SceneEditorInputState& inputState);
	};

	class ScriptingNodeEditor : public UIWidget {
	public:
		ScriptingNodeEditor(ScriptingGizmo& gizmo, UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, uint32_t nodeId, const IScriptNodeType& nodeType, Vector2f pos);

		void onMakeUI() override;
		void onAddedToRoot(UIRoot& root) override;
		void onRemovedFromRoot(UIRoot& root) override;

	protected:
		bool onKeyPress(KeyboardKeyPress key) override;
	
	private:
		ScriptingGizmo& gizmo;
		UIFactory& factory;
		const IEntityEditorFactory& entityEditorFactory;
		uint32_t nodeId;
		const IScriptNodeType& nodeType;
		ConfigNode curSettings;

		void applyChanges();
		void deleteNode();
		void makeFields(const std::shared_ptr<UIWidget>& fieldsRoot);
	};
}
