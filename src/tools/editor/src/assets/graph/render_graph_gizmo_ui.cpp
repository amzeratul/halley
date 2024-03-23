#include "render_graph_gizmo_ui.h"
#include "render_graph_editor.h"

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
	: GraphGizmoUI(std::move(keyboard), std::move(clipboard), graphEditor, std::make_unique<RenderGraphGizmo>(factory, entityEditorFactory, resources, makeRenderGraphTypes()))
{
	renderGraphGizmo = dynamic_cast<RenderGraphGizmo*>(gizmo.get());
}

void RenderGraphGizmoUI::load(BaseGraph& graph)
{
	renderGraphGizmo->setBaseGraph(&graph);
}

std::shared_ptr<GraphNodeTypeCollection> RenderGraphGizmoUI::makeRenderGraphTypes()
{
	auto result = std::make_shared<GraphNodeTypeCollection>();

	result->addNodeType(std::make_unique<RenderGraphNodeType>());

	return result;
}

String RenderGraphNodeType::getId() const
{
	// TODO
	return "todo";
}

String RenderGraphNodeType::getName() const
{
	// TODO
	return "TODO";
}

gsl::span<const IGraphNodeType::PinType> RenderGraphNodeType::getPinConfiguration(const BaseGraphNode& node) const
{
	// TODO
	return {};
}
