#include "graphics/render_target/render_graph.h"
using namespace Halley;

RenderGraphNode::RenderGraphNode()
{}

void RenderGraphNode::connectColourTarget(RenderGraphNode& node, uint8_t outputPin)
{}

void RenderGraphNode::connectDepthTarget(RenderGraphNode& node, uint8_t outputPin)
{}

void RenderGraphNode::connectInput(uint8_t inputPin, RenderGraphNode& node, uint8_t outputPin)
{}

void RenderGraphNode::setPaintMethod(PaintMethod paintMethod)
{}

RenderGraphNode& RenderGraph::addNode()
{
}

RenderGraphNode& RenderGraph::getRenderContextNode()
{
}

void RenderGraph::render(RenderContext& rc)
{
}
