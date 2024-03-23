#pragma once
#include <memory>

#include "halley/graph/base_graph_type.h"

namespace Halley {
	enum class RenderGraphNodeClassification {
		DrawPass,
		Filter,
		Sink,
		Texture
	};

	class RenderGraphNodeType : public IGraphNodeType {
	public:
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;

		virtual RenderGraphNodeClassification getClassification() const = 0;

		Colour4f getColour() const override;
	};

	namespace RenderGraphNodeTypes {
		std::shared_ptr<GraphNodeTypeCollection> makeRenderGraphTypes();

		class PaintNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "paint"; }
			String getName() const override { return "Paint"; }
			RenderGraphNodeClassification getClassification() const override { return RenderGraphNodeClassification::DrawPass; }
		};

		class OverlayNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "overlay"; }
			String getName() const override { return "Overlay"; }
			RenderGraphNodeClassification getClassification() const override { return RenderGraphNodeClassification::Filter; }
		};

		class RenderToTextureNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "renderToTexture"; }
			String getName() const override { return "Render to Texture"; }
			RenderGraphNodeClassification getClassification() const override { return RenderGraphNodeClassification::Sink; }
		};

		class OutputNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "output"; }
			String getName() const override { return "Output"; }
			RenderGraphNodeClassification getClassification() const override { return RenderGraphNodeClassification::Sink; }
		};

		class ImageOutputNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "imageOutput"; }
			String getName() const override { return "Image Output"; }
			RenderGraphNodeClassification getClassification() const override { return RenderGraphNodeClassification::Texture; }
		};
	}
}
