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

		ScriptingBaseGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, const World* world, Resources& resources, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, float baseZoom = 1.0f);
		void setUIRoot(UIRoot& root);
		void setEventSink(UIWidget& eventSink);

		void addNode();
		ScriptNodeId addNode(const String& type, Vector2f pos, ConfigNode settings);
		bool destroyNode(ScriptNodeId id);
		bool destroyNodes(Vector<ScriptNodeId> ids);
		ScriptGraphNode& getNode(ScriptNodeId id);
		const ScriptGraphNode& getNode(ScriptNodeId id) const;

		[[nodiscard]] ConfigNode copySelection() const;
		[[nodiscard]] ConfigNode cutSelection();
		void paste(const ConfigNode& node);
		bool isValidPaste(const ConfigNode& node) const;
		bool deleteSelection();
		void copySelectionToClipboard(const std::shared_ptr<IClipboard>& clipboard) const;
		void cutSelectionToClipboard(const std::shared_ptr<IClipboard>& clipboard);
		void pasteFromClipboard(const std::shared_ptr<IClipboard>& clipboard);

		ScriptGraph& getGraph();
		ScriptGraph* getGraphPtr();
		void setGraph(ScriptGraph* graph);
		void setState(ScriptState* state);
		
		ExecutionQueue& getExecutionQueue();

		void update(Time time, const SceneEditorInputState& inputState);
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

		std::optional<ScriptRenderer::NodeUnderMouseInfo> getNodeUnderMouse() const;
		void setCurNodeDevConData(const String& str);

		void updateNodes();

	private:
		struct Dragging {
			Vector<ScriptNodeId> nodeIds;
			Vector<Vector2f> startPos;
			std::optional<Vector2f> startMousePos;
			bool sticky = false;
			bool hadChange = false;
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
		UIWidget* eventSink = nullptr;

		Vector2f basePos;
		ScriptGraph* scriptGraph = nullptr;
		ScriptState* scriptState = nullptr;

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

		std::optional<std::pair<ScriptRenderer::NodeUnderMouseInfo, String>> devConData;

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
		bool finishAutoConnection();
		std::optional<Connection> findAutoConnectionForPin(ScriptNodeId srcNodeId, ScriptPinId srcPinIdx, Vector2f nodePos, ScriptNodePinType srcPinType, gsl::span<const ScriptNodeId> excludeIds) const;

		void assignNodeTypes() const;
		SelectionSetModifier getSelectionModifier(const SceneEditorInputState& inputState) const;

		void drawWheelGuides(Painter& painter) const;
	};
}
