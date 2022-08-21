#include "script_network.h"

#include "halley/support/logger.h"
using namespace Halley;

String ScriptEntityAuthority::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "has authority over " + getConnectedNodeName(world, node, graph, 0);
}

gsl::span<const IScriptNodeType::PinType> ScriptEntityAuthority::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptEntityAuthority::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Has authority over ");
	str.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	return str.moveResults();
}

ConfigNode ScriptEntityAuthority::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	return ConfigNode(environment.hasNetworkAuthorityOver(readEntityId(environment, node, 0)));
}




String ScriptHostAuthority::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "has host authority";
}

gsl::span<const IScriptNodeType::PinType> ScriptHostAuthority::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptHostAuthority::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	return {"Has host authority", {}};
}

ConfigNode ScriptHostAuthority::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	return ConfigNode(environment.hasHostNetworkAuthority());
}




gsl::span<const IScriptNodeType::PinType> ScriptIfEntityAuthority::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::TargetPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptIfEntityAuthority::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("If has authority over ");
	str.append(getConnectedNodeName(world, node, graph, 1), parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptIfEntityAuthority::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto entityId = readEntityId(environment, node, 1);
	const bool hasAuthority = environment.hasNetworkAuthorityOver(entityId);
	return Result(ScriptNodeExecutionState::Done, 0, hasAuthority ? 1 : 0);
}




gsl::span<const IScriptNodeType::PinType> ScriptIfHostAuthority::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptIfHostAuthority::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	return {"If has host authority", {}};
}

IScriptNodeType::Result ScriptIfHostAuthority::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const bool hasAuthority = environment.hasHostNetworkAuthority();
	return Result(ScriptNodeExecutionState::Done, 0, hasAuthority ? 1 : 0);
}
