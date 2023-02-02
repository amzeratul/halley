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
	return { "Has host authority", {} };
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
	return { "If has host authority", {} };
}

IScriptNodeType::Result ScriptIfHostAuthority::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const bool hasAuthority = environment.hasHostNetworkAuthority();
	return Result(ScriptNodeExecutionState::Done, 0, hasAuthority ? 1 : 0);
}


String ScriptLock::getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 2) {
		return "Lock Acquired";
	} else if (elementIdx == 3) {
		return "Lock Not Acquired";
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
}

gsl::span<const IGraphNodeType::PinType> ScriptLock::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::TargetPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptLock::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Tries to acquire lock for ");
	str.append(getConnectedNodeName(world, node, graph, 1), parameterColour);
	str.append(" and branches based on success");
	return str.moveResults();
}

IScriptNodeType::Result ScriptLock::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	// TODO
	const bool success = true;
	return Result(ScriptNodeExecutionState::Done, 0, success ? 1 : 2);
}



String ScriptLockAvailable::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "Lock for " + getConnectedNodeName(world, node, graph, 0) + " available";
}

gsl::span<const IGraphNodeType::PinType> ScriptLockAvailable::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptLockAvailable::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Is lock for ");
	str.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	str.append(" available or acquired by me?");
	return str.moveResults();
}

ConfigNode ScriptLockAvailable::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	// TODO
	return ConfigNode(true);
}



gsl::span<const IGraphNodeType::PinType> ScriptLockAvailableGate::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::TargetPin, PD::Input }, PinType{ ET::FlowPin, PD::Output, true }, PinType{ ET::FlowPin, PD::Output, true } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptLockAvailableGate::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Flow based on lock for ");
	str.append(getConnectedNodeName(world, node, graph, 1), parameterColour);
	str.append(" being available or acquired by me");
	return str.moveResults();
}

IScriptNodeType::Result ScriptLockAvailableGate::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	// TODO
	return Result(ScriptNodeExecutionState::Done, 0, 1);
}
