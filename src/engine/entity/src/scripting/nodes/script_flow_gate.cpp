#include "script_flow_gate.h"
using namespace Halley;

ScriptFlowGateData::ScriptFlowGateData(const ConfigNode& node)
{
	flowing = node["flowing"].asBool(false);
}

ConfigNode ScriptFlowGateData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["flowing"] = flowing;
	return result;
}

gsl::span<const IScriptNodeType::PinType> ScriptFlowGate::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::FlowPin, PD::Output, true } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptFlowGate::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	const auto desc = getConnectedNodeName(world, node, graph, 1);
	auto str = ColourStringBuilder(true);
	str.append("Flow while ");
	str.append(desc, parameterColour);
	str.append(" is true");
	return str.moveResults();
}

std::pair<String, Vector<ColourOverride>> ScriptFlowGate::getPinDescription(const ScriptGraphNode& node, PinType element, ScriptPinId elementIdx) const
{
	if (elementIdx == 1) {
		return {"Condition", {}};
	} else {
		return ScriptNodeTypeBase<ScriptFlowGateData>::getPinDescription(node, element, elementIdx);
	}
}

void ScriptFlowGate::doInitData(ScriptFlowGateData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data.flowing = false;
}

IScriptNodeType::Result ScriptFlowGate::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptFlowGateData& data) const
{
	const bool shouldFlow = readDataPin(environment, node, 1).asBool(false);

	if (shouldFlow != data.flowing) {
		data.flowing = shouldFlow;
		if (shouldFlow) {
			return Result(ScriptNodeExecutionState::Fork, 0, 1);
		} else {
			return Result(ScriptNodeExecutionState::Executing, time, 0, 1);
		}
	} else {
		return Result(ScriptNodeExecutionState::Executing, time);
	}	
}
