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
	return std::make_unique<RenderGraphNodeDefinition>(node);
}

std::shared_ptr<BaseGraphRenderer> RenderGraphGizmo::makeRenderer(Resources& resources, float baseZoom)
{
	return std::make_shared<RenderGraphRenderer>(resources, *nodeTypes, baseZoom);
}

bool RenderGraphGizmo::canConnectPins(GraphNodePinType src, GraphNodePinType dst) const
{
	if (src.direction == dst.direction) {
		return false;
	}

	if (src.direction == GraphNodePinDirection::Input) {
		std::swap(src, dst);
	}
	const auto typeA = static_cast<RenderGraphElementType>(src.type);
	const auto typeB = static_cast<RenderGraphElementType>(dst.type);

	return typeA == typeB || ((typeA == RenderGraphElementType::ColourBuffer || typeA == RenderGraphElementType::DepthStencilBuffer) && typeB == RenderGraphElementType::Texture);
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

RenderGraphRenderer::RenderGraphRenderer(Resources& resources, const GraphNodeTypeCollection& nodeTypeCollection, float nativeZoom)
	: BaseGraphRenderer(resources, nodeTypeCollection, nativeZoom)
{
}

Colour4f RenderGraphRenderer::getPinColour(GraphNodePinType pinType) const
{
	switch (RenderGraphElementType(pinType.type)) {
	case RenderGraphElementType::Texture:
		return Colour4f(0.75f, 0.75f, 0.99f);
	case RenderGraphElementType::DepthStencilBuffer:
		return Colour4f(0.15f, 0.85f, 0.98f);
	case RenderGraphElementType::ColourBuffer:
		return Colour4f(0.91f, 0.2f, 0.2f);
	case RenderGraphElementType::Dependency:
		return Colour4f(0.91f, 0.55f, 0.2f);
	}

	return Colour4f();
}

Vector2f RenderGraphRenderer::getNodeSize(const IGraphNodeType& nodeType, const BaseGraphNode& node, float curZoom) const
{
	return Vector2f(128.0f, 64.0f);
}
