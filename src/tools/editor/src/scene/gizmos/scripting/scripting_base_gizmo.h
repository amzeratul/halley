#pragma once
#include "halley/graph/base_graph_gizmo.h"

namespace Halley {
	class ScriptingBaseGizmo : public BaseGraphGizmo {
	public:
		ScriptingBaseGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, const World* world, Resources& resources, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, float baseZoom = 1.0f);

		void update(Time time, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter) const override;

		void addNode() override;
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

		std::shared_ptr<UIWidget> makeUI();

		void setEntityTargets(Vector<String> entityTargets);

		void setCurNodeDevConData(const String& str);
		void setDebugDisplayData(HashMap<int, String> values);

		void updateNodes(bool force = false);

	protected:
		bool canDeleteNode(const BaseGraphNode& node) const override;
		bool nodeTypeNeedsSettings(const String& nodeType) const override;

		void openNodeSettings(std::optional<GraphNodeId> nodeId, std::optional<Vector2f> pos, const String& nodeType) override;

		void onNodeAdded(GraphNodeId id) override;

	private:
		std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes;
		const World* world = nullptr;

		ScriptGraph* scriptGraph = nullptr;
		ScriptState* scriptState = nullptr;

		Vector<String> entityTargets;
		ExecutionQueue pendingUITasks;

		std::optional<std::pair<BaseGraphRenderer::NodeUnderMouseInfo, String>> devConData;

		void drawToolTip(Painter& painter, const ScriptGraphNode& node, const BaseGraphRenderer::NodeUnderMouseInfo& nodeInfo) const;
		void drawToolTip(Painter& painter, const String& text, const Vector<ColourOverride>& colours, Vector2f pos) const;

		void assignNodeTypes(bool force = false) const;

		void drawWheelGuides(Painter& painter) const;
	};
}
