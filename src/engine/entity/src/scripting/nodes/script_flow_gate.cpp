#include "script_flow_gate.h"
using namespace Halley;

ScriptFlowGateData::ScriptFlowGateData(const ConfigNode& node)
{
	if (node.hasKey("flowing")) {
		flowing = node["flowing"].asBool();
	}
}

ConfigNode ScriptFlowGateData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	if (flowing) {
		result["flowing"] = *flowing;
	}
	return result;
}

gsl::span<const IScriptNodeType::PinType> ScriptFlowGate::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::FlowPin, PD::Output, true }, PinType{ ET::FlowPin, PD::Output, true } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptFlowGate::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	const auto desc = getConnectedNodeName(world, node, graph, 1);
	auto str = ColourStringBuilder(true);
	str.append("Flow while ");
	str.append(desc, parameterColour);
	return str.moveResults();
}

String ScriptFlowGate::getPinDescription(const ScriptGraphNode& node, PinType element, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "Condition";
	} else if (elementIdx == 2) {
		return "Flow while true";
	} else if (elementIdx == 3) {
		return "Flow while false";
	} else {
		return ScriptNodeTypeBase<ScriptFlowGateData>::getPinDescription(node, element, elementIdx);
	}
}

void ScriptFlowGate::doInitData(ScriptFlowGateData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data.flowing.reset();
}

IScriptNodeType::Result ScriptFlowGate::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptFlowGateData& data) const
{
	const bool shouldFlow = readDataPin(environment, node, 1).asBool(false);

	if (shouldFlow != data.flowing) {
		data.flowing = shouldFlow;
		if (shouldFlow) {
			return Result(ScriptNodeExecutionState::Fork, 0, 1, 2);
		} else {
			return Result(ScriptNodeExecutionState::Fork, 0, 2, 1);
		}
	} else {
		return Result(ScriptNodeExecutionState::Executing, time);
	}	
}


ScriptFlowOnceData::ScriptFlowOnceData(const ConfigNode& node)
{
	if (node.hasKey("active")) {
		active = node["active"].asBool();
	}
}

ConfigNode ScriptFlowOnceData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	if (active) {
		result["active"] = *active;
	}
	return result;
}

gsl::span<const IScriptNodeType::PinType> ScriptFlowOnce::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptFlowOnce::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	const auto desc = getConnectedNodeName(world, node, graph, 1);
	auto str = ColourStringBuilder(true);
	str.append("Flow once");
	return str.moveResults();
}

String ScriptFlowOnce::getPinDescription(const ScriptGraphNode& node, PinType element, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "Flow once";
	}
	else if (elementIdx == 2) {
		return "Flow otherwise";
	}
	else {
		return ScriptNodeTypeBase<ScriptFlowOnceData>::getPinDescription(node, element, elementIdx);
	}
}

void ScriptFlowOnce::doInitData(ScriptFlowOnceData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{

}

IScriptNodeType::Result ScriptFlowOnce::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptFlowOnceData& data) const
{
	if (!data.active) {
		data.active = true;
		return Result(ScriptNodeExecutionState::Done, 0.0);
	}
	return Result(ScriptNodeExecutionState::Done, 0, 2, 0);
}


ScriptLatchData::ScriptLatchData(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		latched = node["latched"].asBool(false);
		if (latched) {
			value = node["value"];
		}
	} else {
		latched = false;
		value = ConfigNode();
	}
}

ConfigNode ScriptLatchData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["latched"] = latched;
	if (latched) {
		result["value"] = value;
	}
	return result;
}

String ScriptLatch::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "latch(" + getConnectedNodeName(world, node, graph, 0) + ")";
}

gsl::span<const IScriptNodeType::PinType> ScriptLatch::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::WriteDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptLatch::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Returns ");
	str.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	str.append(", or last latched value");
	return str.moveResults();
}

void ScriptLatch::doInitData(ScriptLatchData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data = ScriptLatchData(nodeData);
}

ConfigNode ScriptLatch::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptLatchData& data) const
{
	if (data.latched) {
		return ConfigNode(data.value);
	} else {
		return readDataPin(environment, node, 0);
	}
}

void ScriptLatch::doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data, ScriptLatchData& curData) const
{
	const bool newState = data.asBool(false);
	if (curData.latched != newState) {
		curData.latched = newState;
		if (newState) {
			curData.value = readDataPin(environment, node, 0);
		} else {
			curData.value = ConfigNode();
		}
	}
}
