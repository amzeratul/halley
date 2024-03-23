#include "halley/graphics/render_target/render_graph_node_types.h"

using namespace Halley;

gsl::span<const RenderGraphNodeType::PinType> RenderGraphNodeType::getPinConfiguration(const BaseGraphNode& node) const
{
	return {};
}

std::shared_ptr<GraphNodeTypeCollection> RenderGraphNodeTypes::makeRenderGraphTypes()
{
	auto result = std::make_shared<GraphNodeTypeCollection>();

	result->addNodeType(std::make_unique<PaintNodeType>());
	result->addNodeType(std::make_unique<OverlayNodeType>());
	result->addNodeType(std::make_unique<RenderToTextureNodeType>());
	result->addNodeType(std::make_unique<OutputNodeType>());
	result->addNodeType(std::make_unique<ImageOutputNodeType>());

	return result;
}
