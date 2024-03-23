#include "script_logic_gates.h"

using namespace Halley;

String ScriptLogicGateAnd::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto a = getConnectedNodeName(node, graph, 0);
	auto b = getConnectedNodeName(node, graph, 1);
	return addParentheses(std::move(a)) + " AND " + addParentheses(std::move(b));
}

gsl::span<const IScriptNodeType::PinType> ScriptLogicGateAnd::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptLogicGateAnd::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto a = getConnectedNodeName(node, graph, 0);
	auto b = getConnectedNodeName(node, graph, 1);
	ColourStringBuilder result;
	result.append("True if ");
	result.append(addParentheses(std::move(a)), parameterColour);
	result.append(" AND ");
	result.append(addParentheses(std::move(b)), parameterColour);
	return result.moveResults();
}

ConfigNode ScriptLogicGateAnd::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const bool value = readDataPin(environment, node, 0).asBool(false) && readDataPin(environment, node, 1).asBool(false);
	return ConfigNode(value);
}


String ScriptLogicGateOr::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto a = getConnectedNodeName(node, graph, 0);
	auto b = getConnectedNodeName(node, graph, 1);
	return addParentheses(std::move(a)) + " OR " + addParentheses(std::move(b));
}

gsl::span<const IScriptNodeType::PinType> ScriptLogicGateOr::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptLogicGateOr::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto a = getConnectedNodeName(node, graph, 0);
	auto b = getConnectedNodeName(node, graph, 1);
	ColourStringBuilder result;
	result.append("True if ");
	result.append(addParentheses(std::move(a)), parameterColour);
	result.append(" OR ");
	result.append(addParentheses(std::move(b)), parameterColour);
	return result.moveResults();
}

ConfigNode ScriptLogicGateOr::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const bool value = readDataPin(environment, node, 0).asBool(false) || readDataPin(environment, node, 1).asBool(false);
	return ConfigNode(value);
}


String ScriptLogicGateXor::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto a = getConnectedNodeName(node, graph, 0);
	auto b = getConnectedNodeName(node, graph, 1);
	return addParentheses(std::move(a)) + " XOR " + addParentheses(std::move(b));
}

gsl::span<const IScriptNodeType::PinType> ScriptLogicGateXor::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptLogicGateXor::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto a = getConnectedNodeName(node, graph, 0);
	auto b = getConnectedNodeName(node, graph, 1);
	ColourStringBuilder result;
	result.append("True if ");
	result.append(addParentheses(std::move(a)), parameterColour);
	result.append(" XOR ");
	result.append(addParentheses(std::move(b)), parameterColour);
	return result.moveResults();
}

ConfigNode ScriptLogicGateXor::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const bool value = (readDataPin(environment, node, 0).asBool(false) ^ readDataPin(environment, node, 1).asBool(false)) != 0;
	return ConfigNode(value);
}


String ScriptLogicGateNot::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto a = getConnectedNodeName(node, graph, 0);
	return "NOT " + addParentheses(std::move(a));
}

gsl::span<const IScriptNodeType::PinType> ScriptLogicGateNot::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptLogicGateNot::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto a = getConnectedNodeName(node, graph, 0);
	ColourStringBuilder result;
	result.append("True if NOT ");
	result.append(addParentheses(std::move(a)), parameterColour);
	return result.moveResults();
}

ConfigNode ScriptLogicGateNot::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const bool value = !readDataPin(environment, node, 0).asBool(false);
	return ConfigNode(value);
}
