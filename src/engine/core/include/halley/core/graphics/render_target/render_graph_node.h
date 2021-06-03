#pragma once
#include <memory>
#include <vector>


#include "render_graph_definition.h"
#include "render_graph_pin_type.h"
#include "halley/core/graphics/texture_descriptor.h"

namespace Halley {
	class Material;
	class VideoAPI;
	class RenderContext;
	class Texture;
	class RenderGraph;
	class TextureRenderTarget;
	
	class RenderGraphNode {
		friend class RenderGraph;
	
	public:
		explicit RenderGraphNode(const RenderGraphDefinition::Node& definition);
		
	private:
		struct OtherPin {
			RenderGraphNode* node = nullptr;
			uint8_t otherId = 0;
		};
		
		struct InputPin {
			RenderGraphPinType type = RenderGraphPinType::Unknown;
			OtherPin other;
			std::shared_ptr<Texture> texture;
		};

		struct OutputPin {
			RenderGraphPinType type = RenderGraphPinType::Unknown;
			std::vector<OtherPin> others;
		};

		struct Variable {
			String name;
			ConfigNode value;
		};

		void connectInput(uint8_t inputPin, RenderGraphNode& node, uint8_t outputPin);
		void disconnectInput(uint8_t inputPin);

		void startRender();
		void prepareDependencyGraph(VideoAPI& video, Vector2i targetSize);
		void prepareInputPin(InputPin& pin, VideoAPI& video, Vector2i targetSize);
		void allocateVideoResources(VideoAPI& video);
		
		void render(const RenderGraph& graph, VideoAPI& video, const RenderContext& rc, std::vector<RenderGraphNode*>& renderQueue);
		void notifyOutputs(std::vector<RenderGraphNode*>& renderQueue);

		void resetTextures();
		std::shared_ptr<Texture> makeTexture(VideoAPI& video, RenderGraphPinType type);

		void determineIfNeedsRenderTarget();
		
		void renderNode(const RenderGraph& graph, const RenderContext& rc);
		void renderNodePaintMethod(const RenderGraph& graph, const RenderContext& rc);
		void renderNodeOverlayMethod(const RenderGraph& graph, const RenderContext& rc);
		void renderNodeImageOutputMethod(const RenderGraph& graph, const RenderContext& rc);
		RenderContext getTargetRenderContext(const RenderContext& rc) const;

		String id;
		RenderGraphMethod method;

		String paintId;
		String cameraId;
		std::optional<Colour4f> colourClear;
		std::optional<float> depthClear;
		std::optional<uint8_t> stencilClear;

		std::shared_ptr<Material> overlayMethod;
		std::vector<Variable> variables;
		
		bool activeInCurrentPass = false;
		bool ownRenderTarget = false;
		bool passThrough = false;
		int depsLeft = 0;
		Vector2i currentSize;

		std::vector<InputPin> inputPins;
		std::vector<OutputPin> outputPins;

		std::shared_ptr<TextureRenderTarget> renderTarget;
		RenderGraphNode* directOutput = nullptr;
	};
}
