#include "graphics/render_target/render_graph.h"
#include "api/video_api.h"
#include "graphics/render_context.h"
#include "graphics/texture.h"
#include "graphics/material/material.h"
#include "graphics/render_target/render_target.h"
#include "graphics/render_target/render_target_texture.h"
#include "graphics/sprite/sprite.h"
#include "halley/support/logger.h"

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
	
	// Reset if render size changed
	if (currentSize != targetSize) {
		currentSize = targetSize;
		resetTextures();
	}
	
	if (!renderTarget) {
		renderTarget = video.createTextureRenderTarget();
	}

	for (auto& input: inputPins) {
		prepareInputPin(input, video, targetSize);
	}
}

void RenderGraphNode::prepareInputPin(InputPin& input, VideoAPI& video, Vector2i targetSize)
{
	if (input.other.node) {
		// Connected to another node
		++depsLeft;

		if (input.other.node->activeInCurrentPass) {
			if (input.other.node->currentSize != targetSize) {
				throw Exception("Mismatched target sizes", HalleyExceptions::Graphics);
			}
		} else {
			// TODO: scaling
			input.other.node->prepareRender(video, targetSize);
		}
	} else if (input.type != PinType::Texture) {
		// Not connected, assign texture
		if (input.textureId == -1) {
			input.textureId = makeTexture(video, input.type);
		}
	}
}

void RenderGraphNode::resetTextures()
{
	textures.clear();
	renderTarget.reset();
	for (auto& input: inputPins) {
		if (input.textureId >= 0) {
			input.textureId = 0;
		}
	}
}

int8_t RenderGraphNode::makeTexture(VideoAPI& video, PinType type)
{
	Expects (type == PinType::ColourBuffer || type == PinType::DepthStencilBuffer);
	
	const int8_t result = gsl::narrow_cast<int8_t>(textures.size());
	auto& texture = textures.emplace_back(video.createTexture(currentSize));

	auto desc = TextureDescriptor(currentSize, type == PinType::ColourBuffer ? TextureFormat::RGBA : TextureFormat::Depth);
	desc.isRenderTarget = true;
	desc.isDepthStencil = type == PinType::DepthStencilBuffer;
	desc.useFiltering = false; // TODO: allow filtering
	texture->load(std::move(desc));

	return result;
}

void RenderGraphNode::render(RenderContext& rc, std::vector<RenderGraphNode*>& renderQueue)
{
	prepareRenderTarget();
	renderNode(rc);
	notifyOutputs(renderQueue);
}

void RenderGraphNode::prepareRenderTarget()
{
	Expects(renderTarget);
	
	int colourIdx = 0;
	for (const auto& input: inputPins) {
		if (input.type == PinType::ColourBuffer) {
			renderTarget->setTarget(colourIdx++, getInputTexture(input));
		} else if (input.type == PinType::DepthStencilBuffer) {
			renderTarget->setDepthTexture(getInputTexture(input));
		}
	}
}

void RenderGraphNode::renderNode(RenderContext& rc)
{
	rc.with(*renderTarget).bind([=] (Painter& painter)
	{
		if (paintMethod) {
			painter.clear(colourClear, depthClear, stencilClear);
			paintMethod(painter);
		} else if (materialMethod) {
			for (auto& input: inputPins) {
				int idx = 0;
				if (input.type == PinType::Texture) {
					materialMethod->set("tex" + toString(idx++), getInputTexture(input));
				}
			}
			
			const auto& tex = renderTarget->getTexture(0);
			Sprite()
				.setMaterial(materialMethod, false)
				.setSize(Vector2f(currentSize))
				.setTexRect(Rect4f(Vector2f(), Vector2f(currentSize) / Vector2f(tex->getSize())))
				.draw(painter);
		}
	});	
}

std::shared_ptr<Texture> RenderGraphNode::getInputTexture(const InputPin& input)
{
	if (input.textureId >= 0) {
		return textures.at(input.textureId);
	} else if (input.other.node) {
		return input.other.node->getOutputTexture(input.other.otherId);
	} else {
		return {};
	}
}

std::shared_ptr<Texture> RenderGraphNode::getOutputTexture(uint8_t pin)
{
	const auto& output = outputPins.at(pin);
	if (output.type == PinType::ColourBuffer) {
		return renderTarget->getTexture(0);
	} else if (output.type == PinType::DepthStencilBuffer) {
		return renderTarget->getDepthTexture();
	} else {
		throw Exception("Unknown pin type.", HalleyExceptions::Graphics);
	}
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

void RenderGraphNode::setPaintMethod(PaintMethod paintMethod, std::optional<Colour4f> colourClear, std::optional<float> depthClear, std::optional<uint8_t> stencilClear)
{
	this->paintMethod = std::move(paintMethod);
	this->colourClear = colourClear;
	this->depthClear = depthClear;
	this->stencilClear = stencilClear;
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
