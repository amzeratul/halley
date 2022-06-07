#pragma once
#include "halley/data_structures/selection_set.h"
#include "halley/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	class ScriptingBaseGizmo {
	public:
		struct EntityTarget {
			Vector2f pos;
			EntityId entityId;
		};

		using ModifiedCallback = std::function<void()>;

		ScriptingBaseGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, const World* world, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, float baseZoom = 1.0f);
		void setUIRoot(UIRoot& root);

		void addNode();
		ScriptNodeId addNode(const String& type, Vector2f pos, ConfigNode settings);
		bool destroyNode(ScriptNodeId id);
		bool destroyNodes(Vector<ScriptNodeId> ids);
		ScriptGraphNode& getNode(ScriptNodeId id);
		const ScriptGraphNode& getNode(ScriptNodeId id) const;

		ConfigNode copySelection() const;
		ConfigNode cutSelection();
		void paste(const ConfigNode& node);
		bool deleteSelection();

		ScriptGraph& getGraph();
		ScriptGraph* getGraphPtr();
		void setGraph(ScriptGraph* graph);
		
		ExecutionQueue& getExecutionQueue();

		void update(Time time, Resources& resources, const SceneEditorInputState& inputState);
		void draw(Painter& painter) const;

		bool isHighlighted() const;

		std::shared_ptr<UIWidget> makeUI();

		void setZoom(float zoom);
		float getZoom() const;
		void setBasePosition(Vector2f pos);

		void onModified();
		void setModifiedCallback(ModifiedCallback callback);
		void setEntityTargets(Vector<EntityTarget> entityTargets);

		void onMouseWheel(Vector2f mousePos, int amount, KeyMods keyMods);

	private:
		struct Dragging {
			Vector<ScriptNodeId> nodeIds;
			Vector<Vector2f> startPos;
			std::optional<Vector2f> startMousePos;
			bool sticky;
		};

		struct Connection {
			ScriptNodeId srcNode;
			ScriptNodeId dstNode;
			ScriptPinId srcPin;
			ScriptPinId dstPin;
			ScriptNodePinType srcType;
			ScriptNodePinType dstType;
			Vector2f srcPos;
			Vector2f dstPos;
			float distance;

			bool operator<(const Connection& other) const;
			bool conflictsWith(const Connection& connection) const;
		};

		UIFactory& factory;
		const IEntityEditorFactory& entityEditorFactory;
		std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes;
		std::shared_ptr<ScriptRenderer> renderer;

		UIRoot* uiRoot = nullptr;
		const World* world = nullptr;
		Resources* resources = nullptr;

		Vector2f basePos;
		ScriptGraph* scriptGraph = nullptr;

		std::optional<ScriptRenderer::NodeUnderMouseInfo> nodeUnderMouse;
		SelectionSet<ScriptNodeId> selectedNodes;
		std::optional<ScriptRenderer::NodeUnderMouseInfo> nodeEditingConnection;
		std::optional<Vector2f> nodeConnectionDst;
		std::optional<Vector2f> lastMousePos;
		bool lastCtrlHeld = false;
		bool lastShiftHeld = false;

		std::optional<Dragging> dragging;
		Vector<Connection> pendingAutoConnections;

		float zoom = 1.0f;
		float baseZoom = 1.0f;

		Vector<EntityTarget> entityTargets;
		std::optional<size_t> curEntityTarget;

		mutable TextRenderer tooltipLabel;

		ExecutionQueue pendingUITasks;

		ModifiedCallback modifiedCallback;

		void drawToolTip(Painter& painter, const EntityTarget& entityTarget) const;
		void drawToolTip(Painter& painter, const ScriptGraphNode& node, const ScriptRenderer::NodeUnderMouseInfo& nodeInfo) const;
		void drawToolTip(Painter& painter, const String& text, const Vector<ColourOverride>& colours, Vector2f pos) const;
		void drawEntityTargets(Painter& painter) const;

		void openNodeUI(std::optional<ScriptNodeId> nodeId, std::optional<Vector2f> pos, const String& nodeType);

		void onNodeClicked(Vector2f mousePos, SelectionSetModifier modifier);
		void onNodeDragging(const SceneEditorInputState& inputState);
		void onPinClicked(bool rightClick, bool shiftHeld);
		void onEditingConnection(const SceneEditorInputState& inputState);

		void updateNodeAutoConnection(gsl::span<const ScriptNodeId> nodes);
		void pruneConflictingAutoConnections();
		void finishAutoConnection();
		std::optional<Connection> findAutoConnectionForPin(ScriptNodeId srcNodeId, ScriptPinId srcPinIdx, Vector2f nodePos, ScriptNodePinType srcPinType, gsl::span<const ScriptNodeId> excludeIds) const;

		void assignNodeTypes() const;
		SelectionSetModifier getSelectionModifier(const SceneEditorInputState& inputState) const;

		void drawWheelGuides(Painter& painter) const;
	};
}
