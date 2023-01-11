#pragma once
#include "halley/data_structures/selection_set.h"
#include "halley/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	class ScriptingBaseGizmo : public BaseGraphGizmo {
	public:
		struct EntityTarget {
			Vector2f pos;
			EntityId entityId;
		};

		ScriptingBaseGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, const World* world, Resources& resources, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, float baseZoom = 1.0f);

		void addNode();
		GraphNodeId addNode(const String& type, Vector2f pos, ConfigNode settings);
		bool destroyNode(GraphNodeId id);
		bool destroyNodes(Vector<GraphNodeId> ids);
		ScriptGraphNode& getNode(GraphNodeId id);
		const ScriptGraphNode& getNode(GraphNodeId id) const;

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
		void setAutoConnectPins(bool autoConnect);

		ExecutionQueue& getExecutionQueue();

		void update(Time time, const SceneEditorInputState& inputState);
		void draw(Painter& painter) const;

		bool isHighlighted() const;

		std::shared_ptr<UIWidget> makeUI();

		void setEntityTargets(Vector<EntityTarget> entityTargets);

		void onMouseWheel(Vector2f mousePos, int amount, KeyMods keyMods);

		std::optional<BaseGraphRenderer::NodeUnderMouseInfo> getNodeUnderMouse() const;
		void setCurNodeDevConData(const String& str);

		void updateNodes(bool force = false);

	private:
		std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes;
		const World* world = nullptr;
		static constexpr float gridSize = 16.0f;

		ScriptGraph* scriptGraph = nullptr;
		ScriptState* scriptState = nullptr;

		std::optional<BaseGraphRenderer::NodeUnderMouseInfo> nodeUnderMouse;
		SelectionSet<GraphNodeId> selectedNodes;
		std::optional<BaseGraphRenderer::NodeUnderMouseInfo> nodeEditingConnection;
		std::optional<Vector2f> nodeConnectionDst;
		std::optional<Vector2f> lastMousePos;
		bool lastCtrlHeld = false;
		bool lastShiftHeld = false;
		bool autoConnectPin = false;

		std::optional<Dragging> dragging;

		Vector<EntityTarget> entityTargets;
		std::optional<size_t> curEntityTarget;

		ExecutionQueue pendingUITasks;

		std::optional<std::pair<BaseGraphRenderer::NodeUnderMouseInfo, String>> devConData;

		void drawToolTip(Painter& painter, const EntityTarget& entityTarget) const;
		void drawToolTip(Painter& painter, const ScriptGraphNode& node, const BaseGraphRenderer::NodeUnderMouseInfo& nodeInfo) const;
		void drawToolTip(Painter& painter, const String& text, const Vector<ColourOverride>& colours, Vector2f pos) const;
		void drawEntityTargets(Painter& painter) const;

		void openNodeUI(std::optional<GraphNodeId> nodeId, std::optional<Vector2f> pos, const String& nodeType);

		void onNodeClicked(Vector2f mousePos, SelectionSetModifier modifier);
		void onNodeDragging(const SceneEditorInputState& inputState);
		void onPinClicked(bool rightClick, bool shiftHeld);
		void onEditingConnection(const SceneEditorInputState& inputState);

		void assignNodeTypes(bool force = false) const;
		SelectionSetModifier getSelectionModifier(const SceneEditorInputState& inputState) const;

		void drawWheelGuides(Painter& painter) const;
	};
}
