#include "halley/graph/base_graph_type.h"
#include "halley/graph/base_graph.h"
using namespace Halley;


String IGraphNodeType::getLabel(const BaseGraphNode& node) const
{
	return "";
}

Vector<IGraphNodeType::SettingType> IGraphNodeType::getSettingTypes() const
{
	return {};
}

void IGraphNodeType::updateSettings(BaseGraphNode& node, const BaseGraph& graph, Resources& resources) const
{
}

std::pair<String, Vector<ColourOverride>> IGraphNodeType::getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const
{
	return { getName(), {} };
}

String IGraphNodeType::getPinDescription(const BaseGraphNode& node, PinType elementType, uint8_t elementIdx) const
{
	auto getIO = [](GraphNodePinDirection direction) -> const char*
	{
		switch (direction) {
		case GraphNodePinDirection::Input:
			return " Input";
		case GraphNodePinDirection::Output:
			return " Output";
		}
		return nullptr;
	};

	const auto& config = getPinConfiguration(node);
	size_t typeIdx = 0;
	size_t typeTotal = 0;
	for (size_t i = 0; i < config.size(); ++i) {
		if (i == elementIdx) {
			typeIdx = typeTotal;
		}
		if (config[i] == elementType) {
			++typeTotal;
		}
	}

	ColourStringBuilder builder;
	builder.append(getPinTypeName(elementType));
	builder.append(getIO(elementType.direction));
	if (typeTotal > 1) {
		builder.append(" " + toString(static_cast<int>(typeIdx)));
	}
	return builder.moveResults().first;
}

String IGraphNodeType::getPinTypeName(PinType type) const
{
	return "Pin";
}

IGraphNodeType::PinType IGraphNodeType::getPin(const BaseGraphNode& node, size_t n) const
{
	const auto& pins = getPinConfiguration(node);
	if (n < pins.size()) {
		return pins[n];
	}
	return PinType{ {}, GraphNodePinDirection::Input };
}

String IGraphNodeType::getIconName(const BaseGraphNode& node) const
{
	return "";
}
