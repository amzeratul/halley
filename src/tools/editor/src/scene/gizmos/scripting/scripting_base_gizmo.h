#pragma once
#include "halley/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	class ScriptingBaseGizmo {
	public:
		struct EntityTarget {
			Vector2f pos;
			EntityId entityId;
		};

		using ModifiedCallback = std::function<void()>;

		ScriptingBaseGizmo(UIFactory& factory, ISceneEditorWindow& sceneEditorWindow, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes);

		void addNode();
		bool destroyNode(uint32_t id);
		bool destroyHighlightedNode();

		ScriptGraph& getGraph();
		ScriptGraph* getGraphPtr();
		void setGraph(ScriptGraph* graph);
		ScriptGraphNode& getNode(uint32_t id);
		void setBasePosition(Vector2f pos);

		ExecutionQueue& getExecutionQueue();

		void update(Time time, Resources& resources, World* world, const SceneEditorInputState& inputState);
		void draw(Painter& painter) const;
		bool isHighlighted() const;
		std::shared_ptr<UIWidget> makeUI();

		void setZoom(float zoom);
		float getZoom() const;

		void onModified();
		void setModifiedCallback(ModifiedCallback callback);
		void setEntityTargets(Vector<EntityTarget> entityTargets);

	private:
		UIFactory& factory;
		ISceneEditorWindow& sceneEditorWindow;
		std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes;
		std::shared_ptr<ScriptRenderer> renderer;

		Resources* resources = nullptr;

		Vector2f basePos;
		ScriptGraph* scriptGraph = nullptr;
		std::optional<ScriptRenderer::NodeUnderMouseInfo> nodeUnderMouse;
		std::optional<ScriptRenderer::NodeUnderMouseInfo> nodeEditingConnection;
		std::optional<Vector2f> nodeConnectionDst;
		std::optional<Vector2f> lastMousePos;

		bool dragging = false;
		Vector2f startDragPos;
		float zoom = 1.0f;

		Vector<EntityTarget> entityTargets;
		EntityId curEntityTarget;

		mutable TextRenderer tooltipLabel;

		ExecutionQueue pendingUITasks;

		ModifiedCallback modifiedCallback;

		void drawToolTip(Painter& painter, const ScriptGraphNode& node, const ScriptRenderer::NodeUnderMouseInfo& nodeInfo) const;
		void drawEntityTargets(Painter& painter) const;

		void openNodeUI(uint32_t nodeId, std::optional<Vector2f> pos, bool force);
		void addNode(const String& type, Vector2f pos);

		void onNodeClicked(Vector2f mousePos);
		void onNodeDragging(const SceneEditorInputState& inputState);
		void onPinClicked(bool rightClick, bool shiftHeld);
		void onEditingConnection(const SceneEditorInputState& inputState);
	};
}
