#include "render_graph_editor.h"

#include "graph_gizmo_ui.h"
#include "render_graph_gizmo_ui.h"
#include "halley/tools/project/project.h"
#include "src/ui/project_window.h"
using namespace Halley;

RenderGraphEditor::RenderGraphEditor(UIFactory& factory, Resources& gameResources, ProjectWindow& projectWindow, std::shared_ptr<RenderGraphDefinition> graph, AssetEditor* assetEditor)
	: GraphEditor(factory, gameResources, projectWindow, graph, AssetType::RenderGraphDefinition, assetEditor, {}, {})
	, renderGraph(graph)
{
}

std::shared_ptr<GraphGizmoUI> RenderGraphEditor::createGizmoEditor()
{
	return std::make_shared<RenderGraphGizmoUI>(factory, gameResources, *entityEditorFactory, projectWindow.getAPI().input->getKeyboard(), projectWindow.getAPI().system->getClipboard(), *this);
}

RenderGraphAssetEditor::RenderGraphAssetEditor(UIFactory& factory, Resources& gameResoures, ProjectWindow& projectWindow)
	: GraphAssetEditor(factory, gameResoures, projectWindow, AssetType::RenderGraphDefinition)
{
}

std::shared_ptr<BaseGraph> RenderGraphAssetEditor::makeGraph()
{
	return std::make_shared<RenderGraphDefinition>();
}

std::shared_ptr<GraphEditor> RenderGraphAssetEditor::makeGraphEditor(std::shared_ptr<BaseGraph> graph)
{
	return std::make_shared<RenderGraphEditor>(factory, gameResources, projectWindow, std::dynamic_pointer_cast<RenderGraphDefinition>(graph), this);
}
