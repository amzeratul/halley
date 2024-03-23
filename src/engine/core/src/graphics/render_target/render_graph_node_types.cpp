#include "halley/graphics/render_target/render_graph_node_types.h"

using namespace Halley;

gsl::span<const RenderGraphNodeType::PinType> RenderGraphNodeType::getPinConfiguration(const BaseGraphNode& node) const
{
	return {};
}

Colour4f RenderGraphNodeType::getColour() const
{
	switch (getClassification()) {
	case RenderGraphNodeClassification::DrawPass:
		return Colour4f(0.07f, 0.84f, 0.09f);
	case RenderGraphNodeClassification::Filter:
		return Colour4f(0.35f, 0.55f, 0.97f);
	case RenderGraphNodeClassification::Sink:
		return Colour4f(0.97f, 0.35f, 0.35f);
	case RenderGraphNodeClassification::Texture:
		return Colour4f(0.91f, 0.71f, 0.0f);
	}
	return Colour4f(0.2f, 0.2f, 0.2f);
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
