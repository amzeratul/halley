#include "graphics/render_target/render_graph.h"
#include "api/video_api.h"
#include "graphics/render_context.h"
#include "graphics/render_target/render_graph_node.h"
#include "graphics/render_target/render_target.h"
#include "graphics/sprite/sprite.h"

using namespace Halley;


RenderGraph::RenderGraph()
{
	RenderGraphPinType pins[] = { RenderGraphPinType::ColourBuffer, RenderGraphPinType::DepthStencilBuffer };
	addNode(pins, {});
}

RenderGraph::RenderGraph(std::shared_ptr<const RenderGraphDefinition> graphDefinition)
{
	
}

RenderGraphNode& RenderGraph::addNode(gsl::span<const RenderGraphPinType> inputPins, gsl::span<const RenderGraphPinType> outputPins)
{
	return *nodes.emplace_back(std::unique_ptr<RenderGraphNode>(new RenderGraphNode(inputPins, outputPins)));
}

RenderGraphNode& RenderGraph::getRenderContextNode()
{
	return *nodes.at(0);
}

void RenderGraph::render(const RenderContext& rc, VideoAPI& video)
{
	for (auto& node: nodes) {
		node->startRender();
	}

	const auto renderSize = rc.getDefaultRenderTarget().getViewPort().getSize();
	getRenderContextNode().prepareRender(video, renderSize);

	for (auto& node: nodes) {
		node->allocateTextures(video);
	}

	std::vector<RenderGraphNode*> renderQueue;
	renderQueue.reserve(nodes.size());
	for (auto& node: nodes) {
		if (node->activeInCurrentPass && node->depsLeft == 0) {
			renderQueue.push_back(node.get());
		}
	}

	for (size_t i = 0; i < renderQueue.size(); ++i) {
		renderQueue[i]->render(*this, rc, renderQueue);
	}
}

const Camera& RenderGraph::getCamera(std::string_view id) const
{
	return cameras.at(id);
}

void RenderGraph::setCamera(std::string_view id, const Camera& camera)
{
	cameras[id] = camera;
}

const RenderGraph::PaintMethod& RenderGraph::getPaintMethod(std::string_view id) const
{
	return paintMethods.at(id);
}

void RenderGraph::setPaintMethod(std::string_view id, PaintMethod method)
{
	paintMethods[id] = std::move(method);
}
