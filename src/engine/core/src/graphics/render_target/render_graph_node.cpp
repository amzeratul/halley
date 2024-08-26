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
#include "halley/utils/algorithm.h"

using namespace Halley;

RenderGraphNode::RenderGraphNode(const RenderGraphNodeDefinition& definition)
	: id(definition.getSettings().getType() == ConfigNodeType::Map ? definition.getSettings()["name"].asString("") : "")
	, method(fromString<RenderGraphMethod>(definition.getType()))
{
	for (const auto& pin: definition.getPinConfiguration()) {
		if (pin.direction == GraphNodePinDirection::Input) {
			inputPins.emplace_back().type = static_cast<RenderGraphElementType>(pin.type);
		} else {
			outputPins.emplace_back().type = static_cast<RenderGraphElementType>(pin.type);
		}
	}
	
	const auto& pars = definition.getSettings();

	if (method == RenderGraphMethod::Paint) {
		prePaintMethodId = pars["prePaintMethodId"].asString("");
		postPaintMethodId = pars["postPaintMethodId"].asString("");
		paintMasks = pars["paintMasks"].asVector<SpriteMaskBase>({});

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
			for (const auto& [key, value] : pars["variables"].asMap()) {
				variables.emplace_back(Variable{ key, ConfigNode(value) });
			}
		}
	} else if (method == RenderGraphMethod::RenderToTexture) {
		currentSize = pars["renderSize"].asVector2i();
	}
}

void RenderGraphNode::startRender()
{
	activeInCurrentPass = false;
	ownRenderTarget = false;
	canForwardRenderTarget = false;
	depsLeft = 0;
	reuseRenderTarget = nullptr;
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
	for (const auto& other: input.others) {
		// Connected to another node
		++depsLeft;

		if (input.type != RenderGraphElementType::Dependency && other.node->activeInCurrentPass) {
			if (other.node->currentSize != targetSize) {
				throw Exception("Mismatched target sizes", HalleyExceptions::Graphics);
			}
		} else {
			other.node->prepareDependencyGraph(video, targetSize);
		}
	}
}

void RenderGraphNode::determineIfCanForwardRenderTarget()
{
	size_t numColourBufferOutputs = 0;
	size_t numDepthStencilBufferOutputs = 0;

	for (const auto& outputPin: outputPins) {
		for (const auto& otherNode: outputPin.others) {
			if (otherNode.node && otherNode.node->activeInCurrentPass) {
				if (outputPin.type == RenderGraphElementType::ColourBuffer) {
					++numColourBufferOutputs;
				} else if (outputPin.type == RenderGraphElementType::DepthStencilBuffer) {
					++numDepthStencilBufferOutputs;
				}
			}
		}
	}

	canForwardRenderTarget = numColourBufferOutputs == 1 && numDepthStencilBufferOutputs == 1;
}

void RenderGraphNode::determineIfNeedsRenderTarget()
{
	reuseRenderTarget = nullptr;
	if (!activeInCurrentPass) {
		ownRenderTarget = false;
		return;
	}

	// Check if it can reuse render target
	RenderGraphNode* colourInput = nullptr;
	RenderGraphNode* depthStencilInput = nullptr;

	for (const auto& inputPin: inputPins) {
		for (const auto& other: inputPin.others) {
			if (auto* inputNode = other.node) {
				if (inputPin.type == RenderGraphElementType::ColourBuffer) {
					colourInput = inputNode;
				} else if (inputPin.type == RenderGraphElementType::DepthStencilBuffer) {
					depthStencilInput = inputNode;
				}
			}
		}
	}

	const bool isOutput = method == RenderGraphMethod::Output;
	if (colourInput == depthStencilInput && colourInput != nullptr && colourInput->canForwardRenderTarget && !isOutput) {
		reuseRenderTarget = colourInput;
	}

	ownRenderTarget = !reuseRenderTarget && !isOutput;
}

std::shared_ptr<TextureRenderTarget> RenderGraphNode::getRenderTarget(VideoAPI& video)
{
	if (ownRenderTarget) {
		if (!renderTarget) {
			renderTarget = video.createTextureRenderTarget();
		}
	} else if (reuseRenderTarget) {
		renderTarget = reuseRenderTarget->getRenderTarget(video);
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
	if (!reuseRenderTarget) {
		int colourIdx = 0;
		for (auto& input: inputPins) {
			if (renderTarget) {
				// Create Colour/DepthStencil textures for render target, if needed
				if (input.others.empty() && (input.type == RenderGraphElementType::ColourBuffer || input.type == RenderGraphElementType::DepthStencilBuffer)) {
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
	if (const auto* camera = graph.tryGetCamera(cameraId)) {
		getTargetRenderContext(rc).with(*camera).bind([this, &graph] (Painter& painter)
		{
			painter.pushDebugGroup(id);

			if (colourClear || depthClear || stencilClear) {
				painter.clear(colourClear, depthClear, stencilClear);
			}

			if (const auto* prePaintMethod = graph.tryGetPaintMethod(prePaintMethodId)) {
				(*prePaintMethod)(painter);
			}

			for (auto mask: paintMasks) {
				graph.draw(mask, painter);
			}

			if (const auto* postPaintMethod = graph.tryGetPaintMethod(postPaintMethodId)) {
				(*postPaintMethod)(painter);
			}

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

	input.others.push_back({ &node, outputPin });
	output.others.push_back({ this, inputPin });
}

void RenderGraphNode::disconnectInput(uint8_t inputPin)
{
	auto& pin = inputPins.at(inputPin);
	for (auto& other : pin.others) {
		auto* otherNode = other.node;
		auto& outputPin = otherNode->outputPins.at(other.otherId);
		std_ex::erase_if(outputPin.others, [=](const OtherPin& o) { return o.node == this; });
	}
	pin.others = {};
}
