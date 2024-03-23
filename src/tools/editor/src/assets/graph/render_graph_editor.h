#pragma once

#include "graph_asset_editor.h"
#include "graph_editor.h"

namespace Halley {
	class RenderGraphEditor : public GraphEditor {
	public:
		RenderGraphEditor(UIFactory& factory, Resources& gameResources, ProjectWindow& projectWindow, std::shared_ptr<RenderGraphDefinition2> graph, AssetEditor* assetEditor);
		
	protected:
		std::shared_ptr<GraphGizmoUI> createGizmoEditor() override;

	private:
		std::shared_ptr<RenderGraphDefinition2> renderGraph;
	};

	class RenderGraphAssetEditor : public GraphAssetEditor {
	public:
		RenderGraphAssetEditor(UIFactory& factory, Resources& gameResoures, ProjectWindow& projectWindow);

	protected:
		std::shared_ptr<BaseGraph> makeGraph() override;
		std::shared_ptr<GraphEditor> makeGraphEditor(std::shared_ptr<BaseGraph> graph) override;
	};
}
