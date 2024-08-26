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

String RenderGraphNodeType::getPinTypeName(PinType type) const
{
	switch (static_cast<RenderGraphElementType>(type.type)) {
	case RenderGraphElementType::ColourBuffer:
		return "Colour Buffer";
	case RenderGraphElementType::DepthStencilBuffer:
		return "DepthStencil Buffer";
	case RenderGraphElementType::Texture:
		return "Texture";
	case RenderGraphElementType::Dependency:
		return "Dependency";
	}
	return "?";
}

std::pair<String, Vector<ColourOverride>> RenderGraphNodeType::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	ColourStringBuilder str;
	str.append(getName());
	str.append(" ");
	str.append(node.getSettings()["name"].asString(""), settingColour);
	return str.moveResults();
}

String RenderGraphNodeType::getLabel(const BaseGraphNode& node) const
{
	return node.getSettings()["name"].asString("");
}

void RenderGraphNodeType::loadMaterials(RenderGraphNodeDefinition& node, Resources& resources) const
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

Vector<IGraphNodeType::SettingType> RenderGraphNodeTypes::PaintNodeType::getSettingTypes() const
{
	return {
		SettingType{ "name", "Halley::String", Vector<String>{""} },
		SettingType{ "cameraId", "Halley::String", Vector<String>{""} },
		SettingType{ "colourClear", "std::optional<Halley::Colour4f>", Vector<String>{""} },
		SettingType{ "depthClear", "std::optional<float>", Vector<String>{""} },
		SettingType{ "stencilClear", "std::optional<uint8_t>", Vector<String>{""} },
		SettingType{ "prePaintMethodId", "Halley::String", Vector<String>{""} },
		SettingType{ "paintMasks", "Halley::Vector<Halley::SpriteMaskBase>", Vector<String>{""} },
		SettingType{ "postPaintMethodId", "Halley::String", Vector<String>{""} },
	};
}

std::pair<String, Vector<ColourOverride>> RenderGraphNodeTypes::PaintNodeType::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	const auto& settings = node.getSettings();

	ColourStringBuilder str;
	str.append("Paint with cameraId ");
	str.append(settings["cameraId"].asString("undefined"), settingColour);

	if (settings.hasKey("colourClear") || settings.hasKey("depthClear") || settings.hasKey("stencilClear")) {
		str.append("\n");

		if (settings.hasKey("colourClear")) {
			str.append("\nColour: ");
			str.append(settings["colourClear"].asString(), settingColour);
		}
		if (settings.hasKey("depthClear")) {
			str.append("\nDepth: ");
			str.append(settings["depthClear"].asString(), settingColour);
		}
		if (settings.hasKey("stencilClear")) {
			str.append("\nStencil: ");
			str.append(settings["stencilClear"].asString(), settingColour);
		}
	}

	auto prePaint = settings["prePaintMethodId"].asString("");
	auto masks = settings["paintMasks"].asVector<SpriteMaskBase>({});
	auto postPaint = settings["postPaintMethodId"].asString("");

	if (!prePaint.isEmpty() || !masks.empty() || !postPaint.isEmpty()) {
		str.append("\n");

		if (!prePaint.isEmpty()) {
			str.append("\nPre-Paint: ");
			str.append(prePaint, settingColour);
		}

		if (!masks.empty()) {
			str.append("\nDraw Masks: ");
			auto list = String::concat(masks.const_span(), ", ", [](SpriteMaskBase mask) { return toString(mask); });
			str.append("[ " + list + " ]", settingColour);
		}

		if (!postPaint.isEmpty()) {
			str.append("\nPost-Paint: ");
			str.append(postPaint, settingColour);
		}
	}

	return str.moveResults();
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
	const auto& renderNode = dynamic_cast<const RenderGraphNodeDefinition&>(node);
	if (renderNode.getMaterial()) {
		numTexs = renderNode.getMaterial()->getTextureNames().size();
	}

	return gsl::span<const IGraphNodeType::PinType>(data).subspan(0, 4 + numTexs);
}

Vector<IGraphNodeType::SettingType> RenderGraphNodeTypes::OverlayNodeType::getSettingTypes() const
{
	return {
		SettingType{ "name", "Halley::String", Vector<String>{""} },
		SettingType{ "material", "Halley::ResourceReference<Halley::MaterialDefinition>", Vector<String>{""} },
		SettingType{ "variables", "Halley::HashMap<Halley::String, Halley::String>", Vector<String>{""} },
		SettingType{ "colourClear", "std::optional<Halley::Colour4f>", Vector<String>{""} },
		SettingType{ "depthClear", "std::optional<float>", Vector<String>{""} },
		SettingType{ "stencilClear", "std::optional<uint8_t>", Vector<String>{""} }
	};
}

void RenderGraphNodeTypes::OverlayNodeType::loadMaterials(RenderGraphNodeDefinition& node, Resources& resources) const
{
	const auto matId = node.getSettings()["material"].asString("");
	if (!matId.isEmpty()) {
		node.setMaterial(resources.get<MaterialDefinition>(matId));
	} else {
		node.setMaterial({});
	}
}

std::pair<String, Vector<ColourOverride>> RenderGraphNodeTypes::OverlayNodeType::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	const auto& renderGraphNode = dynamic_cast<const RenderGraphNodeDefinition&>(node);
	const auto& settings = node.getSettings();

	ColourStringBuilder str;
	str.append("Overlay with material ");
	str.append(renderGraphNode.getMaterial() ? renderGraphNode.getMaterial()->getName() : "<missing>", settingColour);

	if (settings.hasKey("colourClear") || settings.hasKey("depthClear") || settings.hasKey("stencilClear")) {
		str.append("\n");
		if (settings.hasKey("colourClear")) {
			str.append("\nColour Clear: ");
			str.append(settings["colourClear"].asString(), settingColour);
		}
		if (settings.hasKey("depthClear")) {
			str.append("\nDepth Clear: ");
			str.append(settings["depthClear"].asString(), settingColour);
		}
		if (settings.hasKey("stencilClear")) {
			str.append("\nStencil Clear: ");
			str.append(settings["stencilClear"].asString(), settingColour);
		}
	}
	return str.moveResults();
}

String RenderGraphNodeTypes::OverlayNodeType::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx < 4) {
		return RenderGraphNodeType::getPinDescription(node, elementType, elementIdx);
	}

	const auto& renderGraphNode = dynamic_cast<const RenderGraphNodeDefinition&>(node);
	if (!renderGraphNode.getMaterial()) {
		return "<invalid>";
	}
	return renderGraphNode.getMaterial()->getTextureNames()[elementIdx - 4];
}

gsl::span<const IGraphNodeType::PinType> RenderGraphNodeTypes::RenderToTextureNodeType::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = RenderGraphElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{
		PinType{ ET::ColourBuffer, PD::Input },
		PinType{ ET::Dependency, PD::Output, false, false, true }
	};
	return data;
}

Vector<IGraphNodeType::SettingType> RenderGraphNodeTypes::RenderToTextureNodeType::getSettingTypes() const
{
	return {
		SettingType{ "name", "Halley::String", Vector<String>{""} },
		SettingType{ "renderSize", "Halley::Vector2i", Vector<String>{""} },
	};
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

Vector<IGraphNodeType::SettingType> RenderGraphNodeTypes::OutputNodeType::getSettingTypes() const
{
	return {
		SettingType{ "name", "Halley::String", Vector<String>{""} },
	};
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

Vector<IGraphNodeType::SettingType> RenderGraphNodeTypes::ImageOutputNodeType::getSettingTypes() const
{
	return {
		SettingType{ "name", "Halley::String", Vector<String>{""} },
	};
}
