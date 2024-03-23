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
		virtual RenderGraphNodeClassification getClassification() const = 0;

		Colour4f getColour() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const override;
		String getLabel(const BaseGraphNode& node) const override;

		virtual void loadMaterials(RenderGraphNode2& node, Resources& resources) const;
	};

	namespace RenderGraphNodeTypes {
		std::shared_ptr<GraphNodeTypeCollection> makeRenderGraphTypes();

		class PaintNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "paint"; }
			String getName() const override { return "Paint"; }
			RenderGraphNodeClassification getClassification() const override { return RenderGraphNodeClassification::DrawPass; }

			gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		};

		class OverlayNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "overlay"; }
			String getName() const override { return "Overlay"; }
			RenderGraphNodeClassification getClassification() const override { return RenderGraphNodeClassification::Filter; }

			gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
			void loadMaterials(RenderGraphNode2& node, Resources& resources) const override;
		};

		class RenderToTextureNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "renderToTexture"; }
			String getName() const override { return "Render to Texture"; }
			RenderGraphNodeClassification getClassification() const override { return RenderGraphNodeClassification::Texture; }

			gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		};

		class OutputNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "output"; }
			String getName() const override { return "Output"; }
			RenderGraphNodeClassification getClassification() const override { return RenderGraphNodeClassification::Sink; }

			gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		};

		class ImageOutputNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "imageOutput"; }
			String getName() const override { return "Image Output"; }
			RenderGraphNodeClassification getClassification() const override { return RenderGraphNodeClassification::Sink; }

			gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		};
	}
}
