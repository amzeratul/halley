#pragma once
#include <memory>
#include <vector>

#include "graphics/texture_descriptor.h"

namespace Halley {
	class VideoAPI;
	class RenderSurface;
	class RenderGraph;
	class RenderContext;
	class Painter;
	class Material;
	
	class RenderGraphNode {
		friend class RenderGraph;
	
	public:
		using PaintMethod = std::function<void(Painter&)>;
		enum class PinType {
			Unknown,
			ColourBuffer,
			DepthStencilBuffer,
			Texture
		};
		
		void connectInput(uint8_t inputPin, RenderGraphNode& node, uint8_t outputPin);

		void setPaintMethod(PaintMethod paintMethod);
		void setMaterialMethod(std::shared_ptr<Material> material);

	private:
		struct OtherPin {
			RenderGraphNode* node = nullptr;
			uint8_t otherId = 0;
		};
		
		struct InputPin {
			PinType type = PinType::Unknown;
			OtherPin other;
			int8_t surfaceId = -1;
		};

		struct OutputPin {
			PinType type = PinType::Unknown;
			std::vector<OtherPin> others;
		};
		
		RenderGraphNode(gsl::span<const PinType> inputPins, gsl::span<const PinType> outputPins);

		void startRender();
		void prepareRender(VideoAPI& video, Vector2i targetSize);
		void render(RenderContext& rc, std::vector<RenderGraphNode*>& renderQueue);
		void notifyOutputs(std::vector<RenderGraphNode*>& renderQueue);

		PaintMethod paintMethod;
		std::shared_ptr<Material> materialMethod;
		bool activeInCurrentPass = false;
		int depsLeft = 0;

		std::vector<InputPin> inputPins;
		std::vector<OutputPin> outputPins;
		std::vector<RenderSurface> surfaces;
	};

	class RenderGraph {
	public:
		RenderGraph();
		
		RenderGraphNode& addNode(gsl::span<const RenderGraphNode::PinType> inputPins, gsl::span<const RenderGraphNode::PinType> outputPins);
		RenderGraphNode& getRenderContextNode();

		void render(RenderContext& rc, VideoAPI& video);

	private:
		std::vector<std::unique_ptr<RenderGraphNode>> nodes;
	};
}
