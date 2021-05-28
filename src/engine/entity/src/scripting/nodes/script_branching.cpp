#include "script_branching.h"
using namespace Halley;

gsl::span<const IScriptNodeType::PinType> ScriptBranch::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, std::vector<ColourOverride>> ScriptBranch::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Branch if ");
	str.append(getConnectedNodeName(world, node, graph, 1), Colour4f(0.97f, 0.35f, 0.35f));
	return str.moveResults();
}

IScriptNodeType::Result ScriptBranch::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const bool value = readDataPin(environment, node, 1).asBool(false);
	return Result(ScriptNodeExecutionState::Done, 0, value ? 1 : 2);
}

std::pair<String, std::vector<ColourOverride>> ScriptBranch::getPinDescription(const ScriptGraphNode& node, PinType elementType, uint8_t elementIdx) const
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



gsl::span<const IScriptNodeType::PinType> ScriptFork::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, std::vector<ColourOverride>> ScriptFork::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Fork execution.");
	return str.moveResults();
}

IScriptNodeType::Result ScriptFork::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Done, 0, 1 | 2 | 4);
}



gsl::span<const IScriptNodeType::PinType> ScriptMergeAny::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, std::vector<ColourOverride>> ScriptMergeAny::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Allows ");
	str.append("any", Colour4f(1, 0, 0));
	str.append(" flow threads to continue forward.");
	return str.moveResults();
}

IScriptNodeType::Result ScriptMergeAny::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Done);
}



gsl::span<const IScriptNodeType::PinType> ScriptMergeAll::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, std::vector<ColourOverride>> ScriptMergeAll::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Proceeds with execution only when ");
	str.append("all", Colour4f(1, 0, 0));
	str.append(" connected flows arrive.");
	return str.moveResults();
}

IScriptNodeType::Result ScriptMergeAll::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	auto& counter = environment.getNodeCounter(node.getId());

	size_t expected = 0;
	const auto& pinConfigs = getPinConfiguration();
	const auto& pins = node.getPins();
	for (size_t i = 0; i < pins.size(); ++i) {
		if (pinConfigs[i].type == ScriptNodeElementType::FlowPin && pinConfigs[i].direction == ScriptNodePinDirection::Input) {
			for (const auto& conn: pins[i].connections) {
				if (conn.dstNode) {
					++expected;
				}
			}			
		}
	}

	if (++counter == expected) {
		counter = 0;
		return Result(ScriptNodeExecutionState::Done);
	} else {
		return Result(ScriptNodeExecutionState::Merged);
	}
}
