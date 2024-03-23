#include "render_graph_gizmo_ui.h"
#include "render_graph_editor.h"
#include "halley/graphics/render_target/render_graph_node_types.h"

using namespace Halley;

RenderGraphGizmo::RenderGraphGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, Resources& resources, std::shared_ptr<GraphNodeTypeCollection> nodeTypes)
	: BaseGraphGizmo(factory, entityEditorFactory, resources, std::move(nodeTypes))
{
}

std::unique_ptr<BaseGraphNode> RenderGraphGizmo::makeNode(const ConfigNode& node)
{
	return std::make_unique<RenderGraphNode2>(node);
}

RenderGraphGizmoUI::RenderGraphGizmoUI(UIFactory& factory, Resources& resources, const IEntityEditorFactory& entityEditorFactory, std::shared_ptr<InputKeyboard> keyboard, std::shared_ptr<IClipboard> clipboard, RenderGraphEditor& graphEditor)
	: GraphGizmoUI(std::move(keyboard), std::move(clipboard), graphEditor, std::make_unique<RenderGraphGizmo>(factory, entityEditorFactory, resources, RenderGraphNodeTypes::makeRenderGraphTypes()))
{
	renderGraphGizmo = dynamic_cast<RenderGraphGizmo*>(gizmo.get());
}

void RenderGraphGizmoUI::load(BaseGraph& graph)
{
	renderGraphGizmo->setBaseGraph(&graph);
}
