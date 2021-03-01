#include "graphics/render_target/render_graph.h"

#include "graphics/render_context.h"
#include "graphics/render_target/render_surface.h"
#include "graphics/render_target/render_target.h"
using namespace Halley;

RenderGraphNode::RenderGraphNode(gsl::span<const PinType> input, gsl::span<const PinType> output)
{
	inputPins.resize(input.size());
	for (size_t i = 0; i < input.size(); ++i) {
		inputPins[i].type = input[i];
	}

	outputPins.resize(output.size());
	for (size_t i = 0; i < output.size(); ++i) {
		outputPins[i].type = output[i];
	}
}

void RenderGraphNode::startRender()
{
	activeInCurrentPass = false;
	depsLeft = 0;
}

void RenderGraphNode::prepareRender(VideoAPI& video, Vector2i targetSize)
{
	activeInCurrentPass = true;

	for (auto& input: inputPins) {
		if (input.other.node) {
			// Connected to another node
			++depsLeft;

			if (!input.other.node->activeInCurrentPass) {
				input.other.node->prepareRender(video, targetSize);
			}
		} else if (input.type != PinType::Texture) {
			// Not connected, assign surface
			if (input.surfaceId == -1) {
				input.surfaceId = gsl::narrow_cast<int8_t>(surfaces.size());
				surfaces.emplace_back(video, RenderSurfaceOptions());
			}
			auto& surface = surfaces.at(input.surfaceId);
			surface.setSize(targetSize);
		}
	}
}

void RenderGraphNode::render(RenderContext& rc, std::vector<RenderGraphNode*>& renderQueue)
{
	rc.with(renderTarget).bind([=] (Painter& painter)
	{
		if (paintMethod) {
			paintMethod(painter);
		} else if (materialMethod) {
			// TODO
		}
	});

	notifyOutputs(renderQueue);
}

void RenderGraphNode::notifyOutputs(std::vector<RenderGraphNode*>& renderQueue)
{
	for (const auto& output: outputPins) {
		for (const auto& other: output.others) {
			if (other.node->activeInCurrentPass && --other.node->depsLeft == 0) {
				renderQueue.push_back(other.node);
			}
		}
	}
}

void RenderGraphNode::connectInput(uint8_t inputPin, RenderGraphNode& node, uint8_t outputPin)
{
	auto& input = inputPins.at(inputPin);
	auto& output = node.outputPins.at(outputPin);

	if (input.type != output.type && input.type != PinType::Texture) {
		throw Exception("Incompatible pin types in RenderGraph.", HalleyExceptions::Graphics);
	}
	
	input.other = { &node, outputPin };
	output.others.push_back({ this, inputPin });
}

void RenderGraphNode::setPaintMethod(PaintMethod paintMethod)
{
	this->paintMethod = std::move(paintMethod);
	materialMethod.reset();
}

void RenderGraphNode::setMaterialMethod(std::shared_ptr<Material> material)
{
	materialMethod = std::move(material);
	paintMethod = {};
}



RenderGraph::RenderGraph()
{
	RenderGraphNode::PinType pins[] = { RenderGraphNode::PinType::ColourBuffer, RenderGraphNode::PinType::DepthStencilBuffer };
	addNode(pins, {});
}

RenderGraphNode& RenderGraph::addNode(gsl::span<const RenderGraphNode::PinType> inputPins, gsl::span<const RenderGraphNode::PinType> outputPins)
{
	return *nodes.emplace_back(std::unique_ptr<RenderGraphNode>(new RenderGraphNode(inputPins, outputPins)));
}

RenderGraphNode& RenderGraph::getRenderContextNode()
{
	return *nodes.at(0);
}

void RenderGraph::render(RenderContext& rc, VideoAPI& video)
{
	for (auto& node: nodes) {
		node->startRender();
	}

	const auto renderSize = rc.getDefaultRenderTarget().getViewPort().getSize();
	getRenderContextNode().prepareRender(video, renderSize);

	std::vector<RenderGraphNode*> renderQueue;
	renderQueue.reserve(nodes.size());
	for (auto& node: nodes) {
		if (node->activeInCurrentPass && node->depsLeft == 0) {
			renderQueue.push_back(node.get());
		}
	}

	for (size_t i = 0; i < renderQueue.size(); ++i) {
		renderQueue[i]->render(rc, renderQueue);
	}
}
