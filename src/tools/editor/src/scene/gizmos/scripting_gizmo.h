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
		
		void addNode();
		bool destroyNode(uint32_t id);
		bool destroyHighlightedNode();
		ScriptGraphNode& getNode(uint32_t id);

		ExecutionQueue& getExecutionQueue();

	private:
		struct EntityTarget {
			Vector2f pos;
			EntityId entityId;
		};
		
		UIFactory& factory;
		ISceneEditorWindow& sceneEditorWindow;
		std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes;
		std::shared_ptr<ScriptRenderer> renderer;

		Vector2f basePos;
		ScriptGraph* scriptGraph = nullptr;
		std::optional<ScriptRenderer::NodeUnderMouseInfo> nodeUnderMouse;
		std::optional<ScriptRenderer::NodeUnderMouseInfo> nodeEditingConnection;
		std::optional<Vector2f> nodeConnectionDst;
		std::optional<Vector2f> lastMousePos;

		bool dragging = false;
		Vector2f startDragPos;

		std::vector<EntityTarget> entityTargets;
		EntityId curEntityTarget;

		mutable TextRenderer tooltipLabel;

		ExecutionQueue pendingUITasks;

		void loadEntityData();
		void drawToolTip(Painter& painter, const ScriptGraphNode& node, const ScriptRenderer::NodeUnderMouseInfo& nodeInfo) const;
		void drawEntityTargets(Painter& painter) const;

		void openNodeUI(uint32_t nodeId, std::optional<Vector2f> pos, bool force);
		void addNode(const String& type, Vector2f pos);

		void onNodeClicked(Vector2f mousePos);
		void onNodeDragging(const SceneEditorInputState& inputState);
		void onPinClicked();
		void onEditingConnection(const SceneEditorInputState& inputState);

		void compileEntityTargetList();
	};

	class ScriptingNodeEditor : public UIWidget {
	public:
		ScriptingNodeEditor(ScriptingGizmo& gizmo, UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, uint32_t nodeId, const IScriptNodeType& nodeType, std::optional<Vector2f> pos);

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

	class ScriptingGizmoToolbar : public UIWidget {
	public:
		ScriptingGizmoToolbar(UIFactory& factory, ScriptingGizmo& gizmo);

		void onMakeUI() override;
		void onAddedToRoot(UIRoot& root) override;
		void onRemovedFromRoot(UIRoot& root) override;

	protected:
		bool onKeyPress(KeyboardKeyPress key) override;
	
	private:
		ScriptingGizmo& gizmo;
		UIFactory& factory;
	};
}
