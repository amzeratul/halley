#pragma once
#include <memory>
#include <vector>

#include "graphics/texture_descriptor.h"

namespace Halley {
	class RenderGraph;
	class RenderContext;
	class Painter;
		
	class RenderGraphPin {
	public:
		TextureFormat format = TextureFormat::RGBA;
	};
	
	class RenderGraphNode {
		friend class RenderGraph;
	
	public:
		using PaintMethod = std::function<void(Painter&)>;
		
		void connectColourTarget(RenderGraphNode& node, uint8_t outputPin);
		void connectDepthTarget(RenderGraphNode& node, uint8_t outputPin);
		void connectInput(uint8_t inputPin, RenderGraphNode& node, uint8_t outputPin);

		void setPaintMethod(PaintMethod paintMethod);

	private:
		RenderGraphNode();

		PaintMethod paintMethod;
	};

	class RenderGraph {
	public:
		RenderGraphNode& addNode();
		RenderGraphNode& getRenderContextNode();

		void render(RenderContext& rc);

	private:
		std::vector<std::shared_ptr<RenderGraphNode>> nodes;
		std::shared_ptr<RenderGraphNode> renderContextNode;
	};
}
