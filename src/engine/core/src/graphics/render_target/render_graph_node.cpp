#include "halley/graphics/render_target/render_graph_node.h"

#include <cassert>

#include "halley/graphics/render_target/render_graph.h"
#include "halley/api/video_api.h"
#include "halley/graphics/render_context.h"
#include "halley/graphics/texture.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/render_target/render_target.h"
#include "halley/graphics/render_target/render_target_texture.h"
#include "halley/graphics/sprite/sprite.h"

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
	priority = definition.priority;

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
		
		Vector<RenderGraphPinType> inputPinTypes;
		inputPinTypes.reserve(2 + definition.methodParameters["inputTexCount"].asInt(0));
		inputPinTypes.push_back(RenderGraphPinType::ColourBuffer);
		inputPinTypes.push_back(RenderGraphPinType::DepthStencilBuffer);
		for (size_t i = 0; i < definition.methodParameters["inputTexCount"].asInt(0); ++i) {
			inputPinTypes.push_back(RenderGraphPinType::Texture);
		}

		setPinTypes(inputPins, inputPinTypes);
		setPinTypes(outputPins, { { RenderGraphPinType::ColourBuffer, RenderGraphPinType::DepthStencilBuffer } });
	} else if (method == RenderGraphMethod::Overlay) {
		overlayMethod = std::make_shared<Material>(definition.material);

		if (pars.hasKey("variables")) {
			const auto& seq = pars["variables"].asSequence();
			variables.reserve(seq.size());
			for (const auto& node: seq) {
				variables.emplace_back(Variable{ node["name"].asString(), ConfigNode(node["value"]) });
			}
		}

		const auto& texs = overlayMethod->getDefinition().getTextures();
		Vector<RenderGraphPinType> inputPinTypes;
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
	} else if (method == RenderGraphMethod::RenderToTexture) {
		currentSize = pars["renderSize"].asVector2i();
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

		setPinTypes(outputPins, {{ RenderGraphPinType::Texture }});
	}
}

int RenderGraphNode::getPriority() const
{
	return priority;
}

void RenderGraphNode::startRender()
{
	activeInCurrentPass = false;
	passThrough = false;
	ownRenderTarget = false;
	depsLeft = 0;
	directOutput = nullptr;
}

void RenderGraphNode::prepareDependencyGraph(VideoAPI& video, std::optional<Vector2i> targetSize)
{
	activeInCurrentPass = true;
	
	// Reset if render size changed
	if (targetSize && method != RenderGraphMethod::RenderToTexture) {
		if (currentSize != *targetSize) {
			currentSize = *targetSize;
			resetTextures();
		}
	}

	for (auto& input: inputPins) {
		prepareInputPin(input, video, currentSize);
	}
}

void RenderGraphNode::prepareInputPin(InputPin& input, VideoAPI& video, Vector2i targetSize)
{
	if (input.other.node) {
		// Connected to another node
		++depsLeft;

		if (input.other.node->activeInCurrentPass && input.other.node->method != RenderGraphMethod::RenderToTexture) {
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

	if (method == RenderGraphMethod::RenderToTexture) {
		ownRenderTarget = true;
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

	if (!ownRenderTarget && curOutputNode && !hasOutputPinsWithMultipleConnections) {
		assert(!curOutputNode->passThrough);
		directOutput = curOutputNode;
		directOutput->passThrough = true;
	}
}

std::shared_ptr<TextureRenderTarget> RenderGraphNode::getRenderTarget(VideoAPI& video)
{
	if (ownRenderTarget) {
		if (!renderTarget) {
			renderTarget = video.createTextureRenderTarget();
		}
	} else if (directOutput) {
		renderTarget = directOutput->getRenderTarget(video);
	}
	return renderTarget;
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

	const auto size = Vector2i::max(currentSize, Vector2i(4, 4));
	auto texture = video.createTexture(size);
	texture->setAssetId("renderGraph/" + id + "/" + (type == RenderGraphPinType::ColourBuffer ? "colour" : "depthStencil"));

	auto desc = TextureDescriptor(size, type == RenderGraphPinType::ColourBuffer ? TextureFormat::RGBA : TextureFormat::Depth);
	desc.isRenderTarget = true;
	desc.isDepthStencil = type == RenderGraphPinType::DepthStencilBuffer;
	desc.useFiltering = false; // TODO: allow filtering
	texture->load(std::move(desc));
	texture->setAssetId(id);

	return texture;
}

void RenderGraphNode::render(const RenderGraph& graph, VideoAPI& video, const RenderContext& rc, Vector<RenderGraphNode*>& renderQueue)
{
	prepareTextures(video, rc);
	renderNode(graph, rc);
	notifyOutputs(renderQueue);
}

void RenderGraphNode::prepareTextures(VideoAPI& video, const RenderContext& rc)
{
	getRenderTarget(video);

	if (method == RenderGraphMethod::RenderToTexture) {
		renderTarget->setTarget(0, makeTexture(video, RenderGraphPinType::ColourBuffer));
		renderTarget->setDepthTexture(makeTexture(video, RenderGraphPinType::DepthStencilBuffer));
		return;
	}

	if (!passThrough) {
		int colourIdx = 0;
		for (auto& input: inputPins) {
			if (renderTarget) {
				// Create Colour/DepthStencil textures for render target, if needed
				if (!input.other.node && input.type != RenderGraphPinType::Texture) {
					if (!input.texture) {
						input.texture = makeTexture(video, input.type);
					}
				}

				// Assign textures to render target
				if (input.type == RenderGraphPinType::ColourBuffer && !renderTarget->hasColourBuffer(colourIdx)) {
					renderTarget->setTarget(colourIdx++, input.texture);
				} else if (input.type == RenderGraphPinType::DepthStencilBuffer && !renderTarget->hasDepthBuffer()) {
					renderTarget->setDepthTexture(input.texture);
				}
			} else {
				// No render target, copy instead
				if (input.type == RenderGraphPinType::ColourBuffer && input.texture) {
					renderNodeBlitTexture(input.texture, rc);
				}
			}
		}
	}
}

void RenderGraphNode::renderNode(const RenderGraph& graph, const RenderContext& rc)
{
	if (method == RenderGraphMethod::Paint || method == RenderGraphMethod::RenderToTexture) {
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
			painter.pushDebugGroup(id);
			if (colourClear || depthClear || stencilClear) {
				painter.clear(colourClear, depthClear, stencilClear);
			}
			(*paintMethod)(painter);
			painter.popDebugGroup();
		});
	}
}

void RenderGraphNode::renderNodeOverlayMethod(const RenderGraph& graph, const RenderContext& rc)
{
	const auto& texs = overlayMethod->getDefinition().getTextures();
	size_t idx = 0;
	for (auto& input: inputPins) {
		if (input.type == RenderGraphPinType::Texture) {
			overlayMethod->set(texs.at(idx++).name, input.texture);
		}
	}

	for (const auto& variable: variables) {
		graph.applyVariable(*overlayMethod, variable.name, variable.value);
	}

	const auto camera = Camera(Vector2f(currentSize) * 0.5f);
	getTargetRenderContext(rc).with(camera).bind([=] (Painter& painter)
	{
		painter.pushDebugGroup(id);
		const auto& tex = overlayMethod->getTexture(0);
		Sprite()
			.setMaterial(overlayMethod)
			.setSize(Vector2f(currentSize))
			.setTexRect(Rect4f(Vector2f(), Vector2f(currentSize) / Vector2f(tex->getSize())))
			.draw(painter);
		painter.popDebugGroup();
	});
}

void RenderGraphNode::renderNodeImageOutputMethod(const RenderGraph& graph, const RenderContext& rc)
{
	const auto srcTexture = inputPins.at(0).texture;
	if (srcTexture) {
		auto* img = graph.getImageOutputForNode(id, srcTexture->getSize());
		if (img) {
			getTargetRenderContext(rc).bind([=] (Painter& painter)
			{
				painter.pushDebugGroup(id);
				srcTexture->copyToImage(painter, *img);
				painter.popDebugGroup();
			});
			graph.notifyImage(id);
		}
	}
}

void RenderGraphNode::renderNodeBlitTexture(std::shared_ptr<const Texture> texture, const RenderContext& rc)
{
	getTargetRenderContext(rc).bind([=] (Painter& painter)
	{
		painter.pushDebugGroup(id);
		painter.blitTexture(texture);
		painter.popDebugGroup();
	});
}

RenderContext RenderGraphNode::getTargetRenderContext(const RenderContext& rc) const
{
	if (renderTarget) {
		assert(renderTarget->hasColourBuffer(0));
		return rc.with(*renderTarget);
	} else {
		return rc;
	}
}

void RenderGraphNode::notifyOutputs(Vector<RenderGraphNode*>& renderQueue)
{
	std::shared_ptr<Texture> colour;
	std::shared_ptr<Texture> depthStencil;
	if (renderTarget) {
		colour = renderTarget->getTexture(0);
		depthStencil = renderTarget->getDepthTexture();
	} else {
		for (const auto& input: inputPins) {
			if (input.type == RenderGraphPinType::ColourBuffer) {
				colour = input.texture;
			} else if (input.type == RenderGraphPinType::DepthStencilBuffer) {
				depthStencil = input.texture;
			}
		}
	}
	
	for (const auto& output: outputPins) {
		std::shared_ptr<Texture> texture;
		if (output.type == RenderGraphPinType::ColourBuffer || output.type == RenderGraphPinType::Texture) {
			texture = colour;
		} else if (output.type == RenderGraphPinType::DepthStencilBuffer) {
			texture = depthStencil;
		}
		
		for (const auto& other: output.others) {
			if (other.node->activeInCurrentPass) {
				// TODO: copy when needed
				other.node->inputPins[other.otherId].texture = texture;
				
				if (--other.node->depsLeft == 0) {
					renderQueue.push_back(other.node);
				}
			}
		}
	}
}

const RenderGraphNode::InputPin* RenderGraphNode::getInputPin(int index) const
{
	if (index >= inputPins.size()) {
		return nullptr;
	}
	return &inputPins[index];
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
