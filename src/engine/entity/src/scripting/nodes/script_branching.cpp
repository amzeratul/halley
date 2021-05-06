#include "script_branching.h"
using namespace Halley;

gsl::span<const IScriptNodeType::PinType> ScriptIf::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::DataPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, std::vector<ColourOverride>> ScriptIf::getNodeDescription(const ScriptGraphNode& node, const World& world) const
{
	ColourStringBuilder str;
	str.append("Branch based on \"");
	//str.append(toString(time), Colour4f(0.97f, 0.35f, 0.35f));
	str.append("\".");
	return str.moveResults();
}

IScriptNodeType::Result ScriptIf::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const bool value = readDataPin(environment, node, 1).asBool(false);
	return Result(ScriptNodeExecutionState::Done, 0, value ? 1 : 2);
}

std::pair<String, std::vector<ColourOverride>> ScriptIf::getPinDescription(const ScriptGraphNode& node, PinType elementType, uint8_t elementIdx) const
{
	if (elementIdx >= 1) {
		ColourStringBuilder builder;
		if (elementIdx == 1) {
			builder.append("Condition");
		} else {
			builder.append("Flow Output if ");
			builder.append(elementIdx == 2 ? "true" : "false", Colour4f(1, 0, 0));
		}
		return builder.moveResults();
	} else {
		return IScriptNodeType::getPinDescription(node, elementType, elementIdx);
	}	
}
