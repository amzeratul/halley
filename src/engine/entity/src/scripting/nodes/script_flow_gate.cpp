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
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::FlowPin, PD::Output, true }, PinType{ ET::FlowPin, PD::Output, true } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptFlowGate::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	const auto desc = getConnectedNodeName(world, node, graph, 1);
	auto str = ColourStringBuilder(true);
	str.append("Flow based on ");
	str.append(desc, parameterColour);
	return str.moveResults();
}

std::pair<String, Vector<ColourOverride>> ScriptFlowGate::getPinDescription(const ScriptGraphNode& node, PinType element, ScriptPinId elementIdx) const
{
	if (elementIdx == 1) {
		return {"Condition", {}};
	} else if (elementIdx == 2) {
		return {"Flow while true", {}};
	} else if (elementIdx == 3) {
		return {"Flow while false", {}};
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

String ScriptLatch::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const
{
	return "latch(" + getConnectedNodeName(world, node, graph, 0) + ")";
}

gsl::span<const IScriptNodeType::PinType> ScriptLatch::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
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
