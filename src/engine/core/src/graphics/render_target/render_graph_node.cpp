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
	: id(definition.id)
	, method(definition.method)
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
	} else if (method == RenderGraphMethod::Overlay) {
		overlayMethod = std::make_shared<Material>(definition.material);

		if (pars.hasKey("variables")) {
			const auto& seq = pars["variables"].asSequence();
			variables.reserve(seq.size());
			for (const auto& node: seq) {
				variables.emplace_back(Variable{ node["name"].asString(), ConfigNode(node["value"]) });
			}
		}

		const auto& texs = overlayMethod->getTextureUniforms();
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
		setPinTypes(outputPins, {});
	} else if (method == RenderGraphMethod::ImageOutput) {
		setPinTypes(inputPins, {{ RenderGraphPinType::Texture }});
		setPinTypes(outputPins, {});
	}
}

void RenderGraphNode::startRender()
{
	activeInCurrentPass = false;
	passThrough = false;
	ownRenderTarget = false;
	depsLeft = 0;
}

void RenderGraphNode::prepareDependencyGraph(VideoAPI& video, Vector2i targetSize)
{
	activeInCurrentPass = true;
	
	// Reset if render size changed
	if (currentSize != targetSize) {
		currentSize = targetSize;
		resetTextures();
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
			input.other.node->prepareDependencyGraph(video, targetSize);
		}
	}
}

void RenderGraphNode::determineIfNeedsRenderTarget()
{
	directOutput = nullptr;
	if (!activeInCurrentPass) {
		ownRenderTarget = false;
		return;
	}
	
	// Figure out if we can short-circuit this render context
	bool hasOutputPinsWithMultipleConnections = false;
	bool hasMultipleRenderNodeOutputs = false;
	bool allConnectionsAreCompatible = true;
	RenderGraphNode* curOutputNode = nullptr;
	for (const auto& outputPin: outputPins) {
		int nConnections = 0;
		for (const auto& otherNode: outputPin.others) {
			if (otherNode.node && otherNode.node->activeInCurrentPass) {
				if (otherNode.node != curOutputNode) {
					if (curOutputNode) {
						hasMultipleRenderNodeOutputs = true;
					}
					curOutputNode = otherNode.node;
					if (curOutputNode->inputPins.at(otherNode.otherId).type != outputPin.type) {
						allConnectionsAreCompatible = false;
					}
				}
				++nConnections;
			}
		}
		if (nConnections > 1) {
			hasOutputPinsWithMultipleConnections = true;
		}
	}
	ownRenderTarget = hasOutputPinsWithMultipleConnections || hasMultipleRenderNodeOutputs || !allConnectionsAreCompatible;
	if (!ownRenderTarget && curOutputNode) {
		assert(!curOutputNode->passThrough);
		directOutput = curOutputNode;
		directOutput->passThrough = true;
	}
}

void RenderGraphNode::allocateVideoResources(VideoAPI& video)
{
	if (!activeInCurrentPass) {
		return;
	}
	
	if (ownRenderTarget) {
		if (!renderTarget) {
			renderTarget = video.createTextureRenderTarget();
		}
	} else {
		if (directOutput) {
			renderTarget = directOutput->renderTarget;
		}
	}

	if (renderTarget && !passThrough) {
		int colourIdx = 0;
		for (auto& input: inputPins) {
			if (!input.other.node && input.type != RenderGraphPinType::Texture) {
				if (!input.texture) {
					input.texture = makeTexture(video, input.type);
				}
			}

			if (input.type == RenderGraphPinType::ColourBuffer && !renderTarget->hasColourBuffer(colourIdx)) {
				renderTarget->setTarget(colourIdx++, getInputTexture(input));
			} else if (input.type == RenderGraphPinType::DepthStencilBuffer && !renderTarget->hasDepthBuffer()) {
				renderTarget->setDepthTexture(getInputTexture(input));
			}
		}
	}
}

void RenderGraphNode::resetTextures()
{
	renderTarget.reset();
	for (auto& input: inputPins) {
		input.texture.reset();
	}
}

std::shared_ptr<Texture> RenderGraphNode::makeTexture(VideoAPI& video, RenderGraphPinType type)
{
	Expects (type == RenderGraphPinType::ColourBuffer || type == RenderGraphPinType::DepthStencilBuffer);
	
	auto texture = video.createTexture(currentSize);

	auto desc = TextureDescriptor(currentSize, type == RenderGraphPinType::ColourBuffer ? TextureFormat::RGBA : TextureFormat::Depth);
	desc.isRenderTarget = true;
	desc.isDepthStencil = type == RenderGraphPinType::DepthStencilBuffer;
	desc.useFiltering = false; // TODO: allow filtering
	texture->load(std::move(desc));

	return texture;
}

void RenderGraphNode::render(const RenderGraph& graph, const RenderContext& rc, std::vector<RenderGraphNode*>& renderQueue)
{
	renderNode(graph, rc);
	notifyOutputs(renderQueue);
}

void RenderGraphNode::renderNode(const RenderGraph& graph, const RenderContext& rc)
{
	if (method == RenderGraphMethod::Paint) {
		renderNodePaintMethod(graph, rc);
	} else if (method == RenderGraphMethod::Overlay) {
		renderNodeOverlayMethod(graph, rc);
	} else if (method == RenderGraphMethod::ImageOutput) {
		renderNodeImageOutputMethod(graph, rc);
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

void RenderGraphNode::renderNodeOverlayMethod(const RenderGraph& graph, const RenderContext& rc)
{
	const auto& texs = overlayMethod->getDefinition().getTextures();
	size_t idx = 0;
	for (auto& input: inputPins) {
		if (input.type == RenderGraphPinType::Texture) {
			overlayMethod->set(texs.at(idx++).name, getInputTexture(input));
		}
	}

	for (const auto& variable: variables) {
		graph.applyVariable(*overlayMethod, variable.name, variable.value);
	}

	const auto camera = Camera(Vector2f(currentSize) * 0.5f);
	getTargetRenderContext(rc).with(camera).bind([=] (Painter& painter)
	{
		const auto& tex = overlayMethod->getTexture(0);
		Sprite()
			.setMaterial(overlayMethod, false)
			.setSize(Vector2f(currentSize))
			.setTexRect(Rect4f(Vector2f(), Vector2f(currentSize) / Vector2f(tex->getSize())))
			.draw(painter);
	});
}

void RenderGraphNode::renderNodeImageOutputMethod(const RenderGraph& graph, const RenderContext& rc)
{
	const auto srcTexture = getInputTexture(inputPins.at(0));
	if (srcTexture) {
		auto* img = graph.getImageOutputForNode(id, srcTexture->getSize());
		if (img) {
			srcTexture->copyToImage(*img);
			graph.notifyImage(id);
		}
	}
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
	} else if (input.texture) {
		return input.texture;
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

void RenderGraphNode::disconnectInput(uint8_t inputPin)
{
	auto& pin = inputPins.at(inputPin);
	auto* otherNode = pin.other.node;
	pin.other = OtherPin();

	auto& outputPin = otherNode->outputPins.at(pin.other.otherId);
	auto& outs = outputPin.others;
	outs.erase(std::remove_if(outs.begin(), outs.end(), [=] (const OtherPin& o) { return o.node == this; }), outs.end());
}
