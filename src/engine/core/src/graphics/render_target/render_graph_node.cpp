#include "graphics/render_target/render_graph_node.h"
#include "graphics/render_target/render_graph.h"
#include "api/video_api.h"
#include "graphics/render_context.h"
#include "graphics/texture.h"
#include "graphics/material/material.h"
#include "graphics/material/material_definition.h"
#include "graphics/render_target/render_target.h"
#include "graphics/render_target/render_target_texture.h"
#include "graphics/sprite/sprite.h"

using namespace Halley;

RenderGraphNode::RenderGraphNode(const RenderGraphDefinition::Node& definition)
	: method(definition.method)
{
	auto setPinTypes = [] (auto& pins, gsl::span<const RenderGraphPinType> pinTypes)
	{
		pins.resize(pinTypes.size());
		for (size_t i = 0; i < pinTypes.size(); ++i) {
			pins[i].type = pinTypes[i];
		}
	};
	
	const auto& pars = definition.methodParameters;

	if (method == RenderGraphMethod::Paint) {
		paintId = pars["paintId"].asString();
		cameraId = pars["cameraId"].asString();
		if (pars.hasKey("colourClear")) {
			colourClear = Colour4f::fromString(pars["colourClear"].asString());
		}
		if (pars.hasKey("depthClear")) {
			depthClear = pars["depthClear"].asFloat();
		}
		if (pars.hasKey("stencilClear")) {
			stencilClear = gsl::narrow_cast<uint8_t>(pars["stencilClear"].asInt());
		}
		
		setPinTypes(inputPins, {{ RenderGraphPinType::ColourBuffer, RenderGraphPinType::DepthStencilBuffer }});
		setPinTypes(outputPins, {{ RenderGraphPinType::ColourBuffer, RenderGraphPinType::DepthStencilBuffer }});
	} else if (method == RenderGraphMethod::Screen) {
		screenMethod = std::make_shared<Material>(definition.material);

		if (pars.hasKey("variables")) {
			const auto& seq = pars["variables"].asSequence();
			variables.reserve(seq.size());
			for (const auto& node: seq) {
				variables.emplace_back(Variable{ node["name"].asString(), ConfigNode(node["value"]) });
			}
		}

		const auto& texs = screenMethod->getTextureUniforms();
		std::vector<RenderGraphPinType> inputPinTypes;
		inputPinTypes.reserve(2 + texs.size());
		inputPinTypes.push_back(RenderGraphPinType::ColourBuffer);
		inputPinTypes.push_back(RenderGraphPinType::DepthStencilBuffer);
		for (size_t i = 0; i < texs.size(); ++i) {
			inputPinTypes.push_back(RenderGraphPinType::Texture);
		}
		
		setPinTypes(inputPins, inputPinTypes);
		setPinTypes(outputPins, {{ RenderGraphPinType::ColourBuffer, RenderGraphPinType::DepthStencilBuffer }});
	} else if (method == RenderGraphMethod::Output) {
		setPinTypes(inputPins, {{ RenderGraphPinType::ColourBuffer, RenderGraphPinType::DepthStencilBuffer }});
	}
}

void RenderGraphNode::startRender()
{
	activeInCurrentPass = false;
	passThrough = false;
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

	initializeRenderTarget(video);

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
			input.other.node->prepareRender(video, targetSize);
		}
	}
}

void RenderGraphNode::allocateTextures(VideoAPI& video)
{
	if (activeInCurrentPass && !passThrough) {
		for (auto& input: inputPins) {
			if (!input.other.node && input.type != RenderGraphPinType::Texture) {
				if (input.textureId == -1) {
					input.textureId = makeTexture(video, input.type);
				}
			}
		}
	}
}

void RenderGraphNode::resetTextures()
{
	textures.clear();
	renderTarget.reset();
	for (auto& input: inputPins) {
		if (input.textureId >= 0) {
			input.textureId = -1;
		}
	}
}

void RenderGraphNode::initializeRenderTarget(VideoAPI& video)
{
	// Figure out if we can short-circuit this render context
	bool hasOutputPinsWithMultipleConnections = false;
	bool hasMultipleRenderNodeOutputs = false;
	bool allConnectionsAreCompatible = true;
	RenderGraphNode* curOutputNode = nullptr;
	for (const auto& outputPin: outputPins) {
		if (outputPin.others.size() > 1) {
			hasOutputPinsWithMultipleConnections = true;
		}
		for (const auto& otherNode: outputPin.others) {
			if (otherNode.node != curOutputNode) {
				if (curOutputNode) {
					hasMultipleRenderNodeOutputs = true;
				}
				curOutputNode = otherNode.node;
				if (curOutputNode->inputPins.at(otherNode.otherId).type != outputPin.type) {
					allConnectionsAreCompatible = false;
				}
			}
		}
	}
	const bool needsRenderTarget = curOutputNode != nullptr && (hasOutputPinsWithMultipleConnections || hasMultipleRenderNodeOutputs || !allConnectionsAreCompatible);
	
	if (needsRenderTarget) {
		if (!renderTarget) {
			renderTarget = video.createTextureRenderTarget();
		}
	} else if (curOutputNode) {
		assert(!curOutputNode->passThrough);
		renderTarget = curOutputNode->renderTarget;
		curOutputNode->passThrough = true;
	}
	ownRenderTarget = needsRenderTarget;
}

int8_t RenderGraphNode::makeTexture(VideoAPI& video, RenderGraphPinType type)
{
	Expects (type == RenderGraphPinType::ColourBuffer || type == RenderGraphPinType::DepthStencilBuffer);
	
	const int8_t result = gsl::narrow_cast<int8_t>(textures.size());
	auto& texture = textures.emplace_back(video.createTexture(currentSize));

	auto desc = TextureDescriptor(currentSize, type == RenderGraphPinType::ColourBuffer ? TextureFormat::RGBA : TextureFormat::Depth);
	desc.isRenderTarget = true;
	desc.isDepthStencil = type == RenderGraphPinType::DepthStencilBuffer;
	desc.useFiltering = false; // TODO: allow filtering
	texture->load(std::move(desc));

	return result;
}

void RenderGraphNode::render(const RenderGraph& graph, const RenderContext& rc, std::vector<RenderGraphNode*>& renderQueue)
{
	prepareRenderTargetInputs();
	renderNode(graph, rc);
	notifyOutputs(renderQueue);
}

void RenderGraphNode::prepareRenderTargetInputs()
{
	if (renderTarget && !passThrough) {
		int colourIdx = 0;
		for (const auto& input: inputPins) {
			if (input.type == RenderGraphPinType::ColourBuffer && !renderTarget->hasColourBuffer(colourIdx)) {
				renderTarget->setTarget(colourIdx++, getInputTexture(input));
			} else if (input.type == RenderGraphPinType::DepthStencilBuffer && !renderTarget->hasDepthBuffer()) {
				renderTarget->setDepthTexture(getInputTexture(input));
			}
		}
	}
}

void RenderGraphNode::renderNode(const RenderGraph& graph, const RenderContext& rc)
{
	if (method == RenderGraphMethod::Paint) {
		renderNodePaintMethod(graph, rc);
	} else if (method == RenderGraphMethod::Screen) {
		renderNodeScreenMethod(graph, rc);
	}
}

void RenderGraphNode::renderNodePaintMethod(const RenderGraph& graph, const RenderContext& rc)
{
	const auto* camera = graph.tryGetCamera(cameraId);
	const auto* paintMethod = graph.tryGetPaintMethod(paintId);

	if (camera && paintMethod) {
		getTargetRenderContext(rc).with(*camera).bind([this, paintMethod] (Painter& painter)
		{
			painter.clear(colourClear, depthClear, stencilClear);
			(*paintMethod)(painter);
		});
	}
}

void RenderGraphNode::renderNodeScreenMethod(const RenderGraph& graph, const RenderContext& rc)
{
	const auto& texs = screenMethod->getDefinition().getTextures();
	size_t idx = 0;
	for (auto& input: inputPins) {
		if (input.type == RenderGraphPinType::Texture) {
			screenMethod->set(texs.at(idx++).name, getInputTexture(input));
		}
	}

	for (const auto& variable: variables) {
		graph.applyVariable(*screenMethod, variable.name, variable.value);
	}

	const auto camera = Camera(Vector2f(currentSize) * 0.5f);
	getTargetRenderContext(rc).with(camera).bind([=] (Painter& painter)
	{
		const auto& tex = screenMethod->getTexture(0);
		Sprite()
			.setMaterial(screenMethod, false)
			.setSize(Vector2f(currentSize))
			.setTexRect(Rect4f(Vector2f(), Vector2f(currentSize) / Vector2f(tex->getSize())))
			.draw(painter);
	});
}

RenderContext RenderGraphNode::getTargetRenderContext(const RenderContext& rc) const
{
	if (renderTarget) {
		return rc.with(*renderTarget);
	} else {
		return rc;
	}
}

std::shared_ptr<Texture> RenderGraphNode::getInputTexture(const InputPin& input)
{
	if (input.other.node) {
		return input.other.node->getOutputTexture(input.other.otherId);
	} else if (input.textureId >= 0) {
		return textures.at(input.textureId);
	} else {
		return {};
	}
}

std::shared_ptr<Texture> RenderGraphNode::getOutputTexture(uint8_t pin)
{
	Expects(renderTarget);
	
	const auto& output = outputPins.at(pin);
	if (output.type == RenderGraphPinType::ColourBuffer) {
		return renderTarget->getTexture(0);
	} else if (output.type == RenderGraphPinType::DepthStencilBuffer) {
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

	if (input.type != output.type && input.type != RenderGraphPinType::Texture) {
		throw Exception("Incompatible pin types in RenderGraph.", HalleyExceptions::Graphics);
	}
	
	input.other = { &node, outputPin };
	output.others.push_back({ this, inputPin });
}
