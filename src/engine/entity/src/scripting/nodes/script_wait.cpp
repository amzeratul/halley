#include "script_wait.h"
using namespace Halley;

void ScriptWait::doInitData(ScriptWaitData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	if (nodeData.getType() == ConfigNodeType::Undefined) {
		data.timeLeft = node.getSettings()["time"].asFloat(0.0f);
	} else {
		data.timeLeft = nodeData["time"].asFloat(0);
		data.setFromInput = nodeData["setFromInput"].asBool(false);
	}
}

ScriptWaitData::ScriptWaitData(const ConfigNode& node)
{
	timeLeft = node["time"].asFloat(0);
	setFromInput = node["setFromInput"].asBool(false);
}

ConfigNode ScriptWaitData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType node;
	node["time"] = timeLeft;
	node["setFromInput"] = setFromInput;
	return node;
}

String ScriptWait::getLabel(const ScriptGraphNode& node) const
{
	if (!node.getPin(2).hasConnection()) {
		return toString(node.getSettings()["time"].asFloat(0.0f)) + "s";
	}
	return "";
}

gsl::span<const IScriptNodeType::PinType> ScriptWait::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Input } };
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptWait::getSettingTypes() const
{
	return { SettingType{ "time", "float", Vector<String>{"0"} } };
}

std::pair<String, Vector<ColourOverride>> ScriptWait::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	const float time = node.getSettings()["time"].asFloat(0.0f);
	auto str = ColourStringBuilder(true);
	str.append("Wait ");
	if (node.getPin(2).hasConnection()) {
		str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);

	} else {
		str.append(toString(time), parameterColour);
	}
	str.append(time == 1.0f ? " second" : " seconds");
	return str.moveResults();
}

IScriptNodeType::Result ScriptWait::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptWaitData& curData) const
{
	if (!curData.setFromInput && readDataPin(environment, node, 2).getType() == ConfigNodeType::Float) {
		curData.setFromInput = true;
		curData.timeLeft = readDataPin(environment, node, 2).asFloat();
	}

	const float t = static_cast<float>(time);
	const bool done = t >= curData.timeLeft;
	const float elapsed = done ? curData.timeLeft : t;
	curData.timeLeft -= elapsed;
	return Result(done ? ScriptNodeExecutionState::Done : ScriptNodeExecutionState::Executing, elapsed);
}
