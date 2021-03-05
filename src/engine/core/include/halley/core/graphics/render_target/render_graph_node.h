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
			int8_t textureId = -1;
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

		void startRender();
		void prepareDependencyGraph(VideoAPI& video, Vector2i targetSize);
		void prepareInputPin(InputPin& pin, VideoAPI& video, Vector2i targetSize);
		void allocateTextures(VideoAPI& video);
		
		void render(const RenderGraph& graph, const RenderContext& rc, std::vector<RenderGraphNode*>& renderQueue);
		void notifyOutputs(std::vector<RenderGraphNode*>& renderQueue);

		void resetTextures();
		int8_t makeTexture(VideoAPI& video, RenderGraphPinType type);

		void initializeRenderTarget(VideoAPI& video);
		void prepareRenderTargetInputs();
		
		void renderNode(const RenderGraph& graph, const RenderContext& rc);
		void renderNodePaintMethod(const RenderGraph& graph, const RenderContext& rc);
		void renderNodeScreenMethod(const RenderGraph& graph, const RenderContext& rc);
		RenderContext getTargetRenderContext(const RenderContext& rc) const;

		std::shared_ptr<Texture> getInputTexture(const InputPin& input);
		std::shared_ptr<Texture> getOutputTexture(uint8_t pin);

		RenderGraphMethod method;

		String paintId;
		String cameraId;
		std::optional<Colour4f> colourClear;
		std::optional<float> depthClear;
		std::optional<uint8_t> stencilClear;

		std::shared_ptr<Material> screenMethod;
		std::vector<Variable> variables;
		
		bool activeInCurrentPass = false;
		bool ownRenderTarget = false;
		bool passThrough = false;
		int depsLeft = 0;
		Vector2i currentSize;

		std::vector<InputPin> inputPins;
		std::vector<OutputPin> outputPins;

		std::shared_ptr<TextureRenderTarget> renderTarget;
		std::vector<std::shared_ptr<Texture>> textures;
	};
}
