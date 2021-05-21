#include "script_logic_gates.h"

using namespace Halley;

gsl::span<const IScriptNodeType::PinType> ScriptLogicGateAnd::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, std::vector<ColourOverride>> ScriptLogicGateAnd::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	ColourStringBuilder result;
	result.append("AND: True if all inputs are true.");
	return result.moveResults();
}

ConfigNode ScriptLogicGateAnd::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const bool value = readDataPin(environment, node, 0).asBool(false) && readDataPin(environment, node, 1).asBool(false);
	return ConfigNode(value);
}




gsl::span<const IScriptNodeType::PinType> ScriptLogicGateOr::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, std::vector<ColourOverride>> ScriptLogicGateOr::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	ColourStringBuilder result;
	result.append("OR: True if any of the inputs are true.");
	return result.moveResults();
}

ConfigNode ScriptLogicGateOr::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const bool value = readDataPin(environment, node, 0).asBool(false) || readDataPin(environment, node, 1).asBool(false);
	return ConfigNode(value);
}



gsl::span<const IScriptNodeType::PinType> ScriptLogicGateXor::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, std::vector<ColourOverride>> ScriptLogicGateXor::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	ColourStringBuilder result;
	result.append("XOR: True if exactly one of the inputs is true.");
	return result.moveResults();
}

ConfigNode ScriptLogicGateXor::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const bool value = (readDataPin(environment, node, 0).asBool(false) ^ readDataPin(environment, node, 1).asBool(false)) != 0;
	return ConfigNode(value);
}



gsl::span<const IScriptNodeType::PinType> ScriptLogicGateNot::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, std::vector<ColourOverride>> ScriptLogicGateNot::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	ColourStringBuilder result;
	result.append("NOT: Inverts the boolean value of the input.");
	return result.moveResults();
}

ConfigNode ScriptLogicGateNot::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const bool value = !readDataPin(environment, node, 0).asBool(false);
	return ConfigNode(value);
}
