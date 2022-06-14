#include "script_flow_gate.h"
using namespace Halley;

ScriptFlowGateData::ScriptFlowGateData(const ConfigNode& node)
{
	started = node["started"].asBool(false);
}

ConfigNode ScriptFlowGateData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["started"] = started;
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
	str.append("Allow flow as long as ");
	str.append(desc, Colour4f(0.97f, 0.35f, 0.35f));
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

void ScriptFlowGate::doInitData(ScriptFlowGateData& data, const ScriptGraphNode& node, const ConfigNode& nodeData) const
{
	data.started = false;
}

IScriptNodeType::Result ScriptFlowGate::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptFlowGateData& data) const
{
	const bool condition = readDataPin(environment, node, 1).asBool(false);

	if (!data.started) {
		data.started = true;
		return Result(condition ? ScriptNodeExecutionState::ForkAndConvertToWatcher : ScriptNodeExecutionState::Done, 0, 1);
	} else {
		if (condition) {
			return Result(ScriptNodeExecutionState::Executing, time);
		} else {
			return Result(ScriptNodeExecutionState::Done, 0, 0, 1);
		}
	}	
}
