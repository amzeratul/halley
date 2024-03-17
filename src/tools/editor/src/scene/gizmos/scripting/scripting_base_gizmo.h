#pragma once
#include "halley/graph/base_graph_gizmo.h"

namespace Halley {
	class ScriptingBaseGizmo : public BaseGraphGizmo {
	public:
		ScriptingBaseGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, const World* world, Resources& resources, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, float baseZoom = 1.0f);

		void update(Time time, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter) const override;

		ScriptGraphNode& getNode(GraphNodeId id);
		const ScriptGraphNode& getNode(GraphNodeId id) const;

		ScriptGraph& getGraph();
		ScriptGraph* getGraphPtr();
		void setGraph(ScriptGraph* graph);
		void setState(ScriptState* state);

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
		std::shared_ptr<UIWidget> makeChooseNodeTypeWindow(Vector2f windowSize, UIFactory& factory, Resources& resources, ChooseAssetWindow::Callback callback) override;

	private:
		std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes;
		const World* world = nullptr;

		ScriptGraph* scriptGraph = nullptr;
		ScriptState* scriptState = nullptr;

		Vector<String> entityTargets;

		std::optional<std::pair<BaseGraphRenderer::NodeUnderMouseInfo, String>> devConData;

		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraphRenderer::NodeUnderMouseInfo& nodeInfo) const override;
		void assignNodeTypes(bool force = false) const;
	};
}
