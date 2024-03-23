#include "halley/graphics/render_target/render_graph_node_types.h"

#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/render_target/render_graph_pin_type.h"
#include "halley/resources/resources.h"

using namespace Halley;

Colour4f RenderGraphNodeType::getColour() const
{
	switch (getClassification()) {
	case RenderGraphNodeClassification::DrawPass:
		return Colour4f(0.07f, 0.84f, 0.09f);
	case RenderGraphNodeClassification::Filter:
		return Colour4f(0.35f, 0.55f, 0.97f);
	case RenderGraphNodeClassification::Sink:
		return Colour4f(0.97f, 0.35f, 0.35f);
	case RenderGraphNodeClassification::Texture:
		return Colour4f(0.91f, 0.71f, 0.0f);
	}
	return Colour4f(0.2f, 0.2f, 0.2f);
}

std::pair<String, Vector<ColourOverride>> RenderGraphNodeType::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	ColourStringBuilder str;
	str.append(getName());
	str.append(" ");
	str.append(dynamic_cast<const RenderGraphNode2&>(node).getName(), settingColour);
	return str.moveResults();
}

String RenderGraphNodeType::getLabel(const BaseGraphNode& node) const
{
	return dynamic_cast<const RenderGraphNode2&>(node).getName();
}

void RenderGraphNodeType::loadMaterials(RenderGraphNode2& node, Resources& resources) const
{
}

std::shared_ptr<GraphNodeTypeCollection> RenderGraphNodeTypes::makeRenderGraphTypes()
{
	static std::shared_ptr<GraphNodeTypeCollection> result; // Dodgy

	if (!result) {
		result = std::make_shared<GraphNodeTypeCollection>();

		result->addNodeType(std::make_unique<PaintNodeType>());
		result->addNodeType(std::make_unique<OverlayNodeType>());
		result->addNodeType(std::make_unique<RenderToTextureNodeType>());
		result->addNodeType(std::make_unique<OutputNodeType>());
		result->addNodeType(std::make_unique<ImageOutputNodeType>());
	}

	return result;
}

gsl::span<const IGraphNodeType::PinType> RenderGraphNodeTypes::PaintNodeType::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = RenderGraphElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 5>{
		PinType{ ET::ColourBuffer, PD::Input },
		PinType{ ET::DepthStencilBuffer, PD::Input },
		PinType{ ET::Dependency, PD::Input },
		PinType{ ET::ColourBuffer, PD::Output },
		PinType{ ET::DepthStencilBuffer, PD::Output }
	};
	return data;
}

gsl::span<const IGraphNodeType::PinType> RenderGraphNodeTypes::OverlayNodeType::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = RenderGraphElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 12>{
		PinType{ ET::ColourBuffer, PD::Input },
		PinType{ ET::DepthStencilBuffer, PD::Input },
		PinType{ ET::ColourBuffer, PD::Output },
		PinType{ ET::DepthStencilBuffer, PD::Output },
		PinType{ ET::Texture, PD::Input },
		PinType{ ET::Texture, PD::Input },
		PinType{ ET::Texture, PD::Input },
		PinType{ ET::Texture, PD::Input },
		PinType{ ET::Texture, PD::Input },
		PinType{ ET::Texture, PD::Input },
		PinType{ ET::Texture, PD::Input },
		PinType{ ET::Texture, PD::Input }
	};

	size_t numTexs = 0;
	const auto& renderNode = dynamic_cast<const RenderGraphNode2&>(node);
	if (renderNode.getMaterial()) {
		numTexs = renderNode.getMaterial()->getTextureNames().size();
	}

	return gsl::span<const IGraphNodeType::PinType>(data).subspan(0, 4 + numTexs);
}

void RenderGraphNodeTypes::OverlayNodeType::loadMaterials(RenderGraphNode2& node, Resources& resources) const
{
	const auto matId = node.getSettings()["material"].asString("");
	if (!matId.isEmpty()) {
		node.setMaterial(resources.get<MaterialDefinition>(matId));
	} else {
		node.setMaterial({});
	}
}

gsl::span<const IGraphNodeType::PinType> RenderGraphNodeTypes::RenderToTextureNodeType::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = RenderGraphElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{
		PinType{ ET::ColourBuffer, PD::Input },
		PinType{ ET::Dependency, PD::Output }
	};
	return data;
}

gsl::span<const IGraphNodeType::PinType> RenderGraphNodeTypes::OutputNodeType::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = RenderGraphElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{
		PinType{ ET::ColourBuffer, PD::Input },
		PinType{ ET::DepthStencilBuffer, PD::Input }
	};
	return data;
}

gsl::span<const IGraphNodeType::PinType> RenderGraphNodeTypes::ImageOutputNodeType::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = RenderGraphElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 1>{
		PinType{ ET::ColourBuffer, PD::Input }
	};
	return data;
}
