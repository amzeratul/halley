#pragma once
#include <memory>
#include <vector>

#include "graphics/texture_descriptor.h"

namespace Halley {
	class RenderGraph;
	class RenderContext;
	class Painter;
	class Material;
		
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
		void setMaterialMethod(std::shared_ptr<Material> material);

	private:
		RenderGraphNode();

		PaintMethod paintMethod;
		std::shared_ptr<Material> materialMethod;
	};

	class RenderGraph {
	public:
		RenderGraph();
		
		RenderGraphNode& addNode();
		RenderGraphNode& getRenderContextNode();

		void render(RenderContext& rc);

	private:
		std::vector<std::unique_ptr<RenderGraphNode>> nodes;
		std::unique_ptr<RenderGraphNode> renderContextNode;
	};
}
