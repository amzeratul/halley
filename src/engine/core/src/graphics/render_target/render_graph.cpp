#include "graphics/render_target/render_graph.h"
#include "api/video_api.h"
#include "graphics/render_context.h"
#include "graphics/render_target/render_graph_definition.h"
#include "graphics/render_target/render_graph_node.h"
#include "graphics/render_target/render_target.h"
#include "graphics/sprite/sprite.h"

using namespace Halley;


RenderGraph::RenderGraph()
{
	addOutputNode();
}

RenderGraph::RenderGraph(std::shared_ptr<const RenderGraphDefinition> graphDefinition)
{
	addOutputNode();
	
	for (const auto& nodeDefinition: graphDefinition->getNodes()) {
		addNode(nodeDefinition.id, std::make_unique<RenderGraphNode>(nodeDefinition));
	}
	for (const auto& connectionDefinition: graphDefinition->getConnections()) {
		auto* from = getNode(connectionDefinition.fromId);
		auto* to = getNode(connectionDefinition.toId);

		to->connectInput(connectionDefinition.toPin, *from, connectionDefinition.fromPin);
	}
}

void RenderGraph::addNode(String id, std::unique_ptr<RenderGraphNode> node)
{
	if (nodeMap.find(id) != nodeMap.end()) {
		throw Exception("Duplicate id \"" + id + "\" in RenderGraph.", HalleyExceptions::Graphics);
	}
	
	nodes.emplace_back(std::move(node));
	nodeMap[std::move(id)] = nodes.back().get();
}

void RenderGraph::addOutputNode()
{
	RenderGraphDefinition::Node nodeDef;
	nodeDef.method = RenderGraphMethod::Output;
	addNode("output", std::make_unique<RenderGraphNode>(nodeDef));
}

RenderGraphNode* RenderGraph::getNode(const String& id)
{
	return nodeMap.at(id);
}

void RenderGraph::render(const RenderContext& rc, VideoAPI& video)
{
	for (auto& node: nodes) {
		node->startRender();
	}

	const auto renderSize = rc.getDefaultRenderTarget().getViewPort().getSize();
	auto* outputNode = getNode("output");
	outputNode->prepareRender(video, renderSize);

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
