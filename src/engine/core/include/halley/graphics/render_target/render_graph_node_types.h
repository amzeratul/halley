#pragma once
#include <memory>

#include "halley/graph/base_graph_type.h"

namespace Halley {
	class RenderGraphNodeType : public IGraphNodeType {
	public:
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
	};

	namespace RenderGraphNodeTypes {
		std::shared_ptr<GraphNodeTypeCollection> makeRenderGraphTypes();

		class PaintNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "paint"; }
			String getName() const override { return "Paint"; }
		};

		class OverlayNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "overlay"; }
			String getName() const override { return "Overlay"; }
		};

		class RenderToTextureNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "renderToTexture"; }
			String getName() const override { return "Render to Texture"; }
		};

		class OutputNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "output"; }
			String getName() const override { return "Output"; }
		};

		class ImageOutputNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "imageOutput"; }
			String getName() const override { return "Image Output"; }
		};
	}
}
