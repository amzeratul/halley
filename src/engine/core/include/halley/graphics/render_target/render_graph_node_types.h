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
		String getPinTypeName(PinType type) const override;

		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const override;
		String getLabel(const BaseGraphNode& node) const override;

		virtual void loadMaterials(RenderGraphNodeDefinition& node, Resources& resources) const;
	};

	namespace RenderGraphNodeTypes {
		std::shared_ptr<GraphNodeTypeCollection> makeRenderGraphTypes();

		class PaintNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "paint"; }
			String getName() const override { return "Paint"; }
			String getIconName(const BaseGraphNode& node) const override { return "render_graph_icons/paint.png"; }
			RenderGraphNodeClassification getClassification() const override { return RenderGraphNodeClassification::DrawPass; }

			gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
			std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const override;
		};

		class OverlayNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "overlay"; }
			String getName() const override { return "Overlay"; }
			String getIconName(const BaseGraphNode& node) const override { return "render_graph_icons/overlay.png"; }
			RenderGraphNodeClassification getClassification() const override { return RenderGraphNodeClassification::Filter; }

			gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
			void loadMaterials(RenderGraphNodeDefinition& node, Resources& resources) const override;
			std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const override;
			String getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		};

		class RenderToTextureNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "renderToTexture"; }
			String getName() const override { return "Render to Texture"; }
			String getIconName(const BaseGraphNode& node) const override { return "render_graph_icons/render_to_texture.png"; }
			RenderGraphNodeClassification getClassification() const override { return RenderGraphNodeClassification::Texture; }

			gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		};

		class OutputNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "output"; }
			String getName() const override { return "Output"; }
			String getIconName(const BaseGraphNode& node) const override { return "render_graph_icons/output.png"; }
			RenderGraphNodeClassification getClassification() const override { return RenderGraphNodeClassification::Sink; }

			gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		};

		class ImageOutputNodeType : public RenderGraphNodeType {
		public:
			String getId() const override { return "imageOutput"; }
			String getName() const override { return "Image Output"; }
			String getIconName(const BaseGraphNode& node) const override { return "render_graph_icons/image_output.png"; }
			RenderGraphNodeClassification getClassification() const override { return RenderGraphNodeClassification::Sink; }

			gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		};
	}
}
