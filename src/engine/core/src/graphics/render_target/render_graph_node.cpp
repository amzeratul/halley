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

RenderGraphNode::RenderGraphNode(const RenderGraphNodeDefinition& definition)
	: id(definition.getName())
	, method(fromString<RenderGraphMethod>(definition.getType()))
{
	auto setPinTypes = [] (auto& pins, gsl::span<const RenderGraphElementType> pinTypes)
	{
		pins.resize(pinTypes.size());
		for (size_t i = 0; i < pinTypes.size(); ++i) {
			pins[i].type = pinTypes[i];
		}
	};
	
	const auto& pars = definition.getSettings();
	//priority = definition.priority;

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

		setPinTypes(inputPins, {{ RenderGraphElementType::ColourBuffer, RenderGraphElementType::DepthStencilBuffer, RenderGraphElementType::Dependency }});
		setPinTypes(outputPins, { { RenderGraphElementType::ColourBuffer, RenderGraphElementType::DepthStencilBuffer } });
	} else if (method == RenderGraphMethod::Overlay) {
		overlayMethod = std::make_shared<Material>(definition.getMaterial());
		if (pars.hasKey("colourClear")) {
			colourClear = Colour4f::fromString(pars["colourClear"].asString());
		}
		if (pars.hasKey("depthClear")) {
			depthClear = pars["depthClear"].asFloat();
		}
		if (pars.hasKey("stencilClear")) {
			stencilClear = gsl::narrow_cast<uint8_t>(pars["stencilClear"].asInt());
		}

		if (pars.hasKey("variables")) {
			const auto& seq = pars["variables"].asSequence();
			variables.reserve(seq.size());
			for (const auto& node: seq) {
				variables.emplace_back(Variable{ node["name"].asString(), ConfigNode(node["value"]) });
			}
		}

		const auto& texs = overlayMethod->getDefinition().getTextures();
		Vector<RenderGraphElementType> inputPinTypes;
		inputPinTypes.reserve(2 + texs.size());
		inputPinTypes.push_back(RenderGraphElementType::ColourBuffer);
		inputPinTypes.push_back(RenderGraphElementType::DepthStencilBuffer);
		for (size_t i = 0; i < texs.size(); ++i) {
			inputPinTypes.push_back(RenderGraphElementType::Texture);
		}
		
		setPinTypes(inputPins, inputPinTypes);
		setPinTypes(outputPins, {{ RenderGraphElementType::ColourBuffer, RenderGraphElementType::DepthStencilBuffer }});
	} else if (method == RenderGraphMethod::Output) {
		setPinTypes(inputPins, {{ RenderGraphElementType::ColourBuffer, RenderGraphElementType::DepthStencilBuffer }});
		setPinTypes(outputPins, {});
	} else if (method == RenderGraphMethod::ImageOutput) {
		setPinTypes(inputPins, {{ RenderGraphElementType::Texture }});
		setPinTypes(outputPins, {});
	} else if (method == RenderGraphMethod::RenderToTexture) {
		currentSize = pars["renderSize"].asVector2i();
		setPinTypes(inputPins, {{ RenderGraphElementType::ColourBuffer }});
		setPinTypes(outputPins, {{ RenderGraphElementType::Dependency }});
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
			renderTarget.reset();
		}
	}

	for (auto& input: inputPins) {
		prepareInputPin(input, video, currentSize);
	}
}

void RenderGraphNode::prepareInputPin(InputPin& input, VideoAPI& video, Vector2i targetSize)
{
	if (input.type == RenderGraphElementType::Dependency) {
		for (auto& dependency : input.dependencies) {
			if (!dependency.node) {
				continue;
			}

			++depsLeft;
			if (dependency.node->activeInCurrentPass && dependency.node->method != RenderGraphMethod::RenderToTexture) {
				if (dependency.node->currentSize != targetSize) {
					throw Exception("Mismatched target sizes", HalleyExceptions::Graphics);
				}
			}
			else {
				dependency.node->prepareDependencyGraph(video, targetSize);
			}
		}

		return;
	}

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

	// Figure out if we can short-circuit this render context
	bool hasOutputPinsWithMultipleConnections = false;
	bool hasMultipleRenderNodeOutputs = false;
	bool allConnectionsAreCompatible = true;
	bool isDependency = false;
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
					if (curOutputNode->inputPins.at(otherNode.otherId).type == outputPin.type && outputPin.type == RenderGraphElementType::Dependency) {
						isDependency = true;
					}
				}
				++nConnections;
			}
		}
		if (nConnections > 1) {
			hasOutputPinsWithMultipleConnections = true;
		}
	}
	
	ownRenderTarget = hasOutputPinsWithMultipleConnections || hasMultipleRenderNodeOutputs || !allConnectionsAreCompatible || isDependency;

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

std::shared_ptr<Texture> RenderGraphNode::makeTexture(VideoAPI& video, RenderGraphElementType type)
{
	Expects (type == RenderGraphElementType::ColourBuffer || type == RenderGraphElementType::DepthStencilBuffer);

	const auto size = Vector2i::max(currentSize, Vector2i(4, 4));
	auto texture = video.createTexture(size);
	texture->setAssetId("renderGraph/" + id + "/" + (type == RenderGraphElementType::ColourBuffer ? "colour" : "depthStencil"));

	auto desc = TextureDescriptor(size, type == RenderGraphElementType::ColourBuffer ? TextureFormat::RGBA : TextureFormat::Depth);
	desc.isRenderTarget = true;
	desc.isDepthStencil = type == RenderGraphElementType::DepthStencilBuffer;
	desc.useFiltering = false; // TODO: allow filtering
	texture->load(std::move(desc));

	return texture;
}

void RenderGraphNode::updateTexture(std::shared_ptr<Texture> texture, RenderGraphElementType type)
{
	auto desc = TextureDescriptor(currentSize, type == RenderGraphElementType::ColourBuffer ? TextureFormat::RGBA : TextureFormat::Depth);
	desc.isRenderTarget = true;
	desc.isDepthStencil = type == RenderGraphElementType::DepthStencilBuffer;
	desc.useFiltering = false;
	texture->load(std::move(desc));
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

	bool updated = false;
	if (!passThrough) {
		int colourIdx = 0;
		for (auto& input: inputPins) {
			if (renderTarget) {
				// Create Colour/DepthStencil textures for render target, if needed
				if (!input.other.node && input.type != RenderGraphElementType::Texture && input.type != RenderGraphElementType::Dependency) {
					if (!input.texture) {
						updated = true;
						input.texture = makeTexture(video, input.type);
					} else {
						if (input.texture->getSize() != currentSize) {
							updateTexture(input.texture, input.type);
						}
					}
				}

				// Assign textures to render target
				if (input.type == RenderGraphElementType::ColourBuffer && (!renderTarget->hasColourBuffer(colourIdx) || updated)) {
					renderTarget->setTarget(colourIdx++, input.texture);
				} else if (input.type == RenderGraphElementType::DepthStencilBuffer && (!renderTarget->hasDepthBuffer() || updated)) {
					renderTarget->setDepthTexture(input.texture);
				}
			} else {
				// No render target, copy instead
				if (input.type == RenderGraphElementType::ColourBuffer && input.texture) {
					renderNodeBlitTexture(input.texture, rc);
				}
			}
		}
	}
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
		if (input.type == RenderGraphElementType::Texture) {
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
		if (colourClear || depthClear || stencilClear) {
			painter.clear(colourClear, depthClear, stencilClear);
		}
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
			if (input.type == RenderGraphElementType::ColourBuffer) {
				colour = input.texture;
			} else if (input.type == RenderGraphElementType::DepthStencilBuffer) {
				depthStencil = input.texture;
			}
		}
	}
	
	for (const auto& output: outputPins) {
		std::shared_ptr<Texture> texture;
		if (output.type == RenderGraphElementType::ColourBuffer || output.type == RenderGraphElementType::Texture) {
			texture = colour;
		} else if (output.type == RenderGraphElementType::DepthStencilBuffer) {
			texture = depthStencil;
		}
		
		for (const auto& other: output.others) {
			if (other.node->activeInCurrentPass) {
				// TODO: copy when needed
				if (other.node->inputPins[other.otherId].type != RenderGraphElementType::Dependency) {
					other.node->inputPins[other.otherId].texture = texture;
				}
				
				if (--other.node->depsLeft == 0) {
					renderQueue.push_back(other.node);
				}
			}
		}
	}
}

void RenderGraphNode::connectInput(uint8_t inputPin, RenderGraphNode& node, uint8_t outputPin)
{
	auto& input = inputPins.at(inputPin);
	auto& output = node.outputPins.at(outputPin);

	if (input.type != output.type && input.type != RenderGraphElementType::Texture) {
		throw Exception("Incompatible pin types in RenderGraph.", HalleyExceptions::Graphics);
	}

	if (input.type == RenderGraphElementType::Dependency) {
		input.dependencies.push_back({ &node, outputPin });
	}

	input.other = { &node, outputPin };
	output.others.push_back({ this, inputPin });
}

void RenderGraphNode::disconnectInput(uint8_t inputPin)
{
	auto& pin = inputPins.at(inputPin);
	auto* otherNode = pin.other.node;
	if (otherNode == nullptr) {
		return;
	}

	auto& outputPin = otherNode->outputPins.at(pin.other.otherId);
	auto& outs = outputPin.others;
	outs.erase(std::remove_if(outs.begin(), outs.end(), [=] (const OtherPin& o) { return o.node == this; }), outs.end());
	pin.other = OtherPin();
}
