#include "graphics/render_target/render_graph.h"
using namespace Halley;

RenderGraphNode::RenderGraphNode()
{
	// TODO
}

void RenderGraphNode::connectColourTarget(RenderGraphNode& node, uint8_t outputPin)
{
	// TODO
}

void RenderGraphNode::connectDepthTarget(RenderGraphNode& node, uint8_t outputPin)
{
	// TODO
}

void RenderGraphNode::connectInput(uint8_t inputPin, RenderGraphNode& node, uint8_t outputPin)
{
	// TODO
}

void RenderGraphNode::setPaintMethod(PaintMethod paintMethod)
{
	this->paintMethod = std::move(paintMethod);
}

void RenderGraphNode::setMaterialMethod(std::shared_ptr<Material> material)
{
	materialMethod = std::move(material);
}


RenderGraph::RenderGraph()
{
	renderContextNode = std::unique_ptr<RenderGraphNode>(new RenderGraphNode());
}

RenderGraphNode& RenderGraph::addNode()
{
	return *nodes.emplace_back(std::unique_ptr<RenderGraphNode>(new RenderGraphNode()));
}

RenderGraphNode& RenderGraph::getRenderContextNode()
{
	return *renderContextNode;
}

void RenderGraph::render(RenderContext& rc)
{
	// TODO
}
