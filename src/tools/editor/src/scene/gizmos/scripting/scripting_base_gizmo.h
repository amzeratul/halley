#pragma once
#include "halley/graph/base_graph_gizmo.h"

namespace Halley {
	class ScriptingBaseGizmo : public BaseGraphGizmo {
	public:
		ScriptingBaseGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, Resources& resources, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, float baseZoom = 1.0f);

		ScriptGraph& getGraph();
		void setGraph(ScriptGraph* graph);
		void setState(ScriptState* state);

		std::shared_ptr<UIWidget> makeUI() override;

		void setEntityTargets(Vector<String> entityTargets);

		void setCurNodeDevConData(const String& str);
		void setDebugDisplayData(HashMap<int, String> values);

	protected:
		bool canDeleteNode(const BaseGraphNode& node) const override;
		bool nodeTypeNeedsSettings(const String& nodeType) const override;
		void openNodeSettings(std::optional<GraphNodeId> nodeId, std::optional<Vector2f> pos, const String& nodeType) override;
		std::shared_ptr<UIWidget> makeChooseNodeTypeWindow(Vector2f windowSize, UIFactory& factory, Resources& resources, ChooseAssetWindow::Callback callback) override;
		std::unique_ptr<BaseGraphNode> makeNode(const ConfigNode& node) override;
		std::shared_ptr<BaseGraphRenderer> makeRenderer(Resources& resources, float baseZoom) override;

	private:
		ScriptGraph* scriptGraph = nullptr;
		ScriptState* scriptState = nullptr;

		Vector<String> entityTargets;
		std::optional<std::pair<BaseGraphRenderer::NodeUnderMouseInfo, String>> devConData;

		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraphRenderer::NodeUnderMouseInfo& nodeInfo) const override;
	};
}
