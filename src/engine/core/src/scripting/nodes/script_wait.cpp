#include "script_wait.h"
using namespace Halley;

void ScriptWait::doInitData(ScriptWaitData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data = ScriptWaitData(nodeData, node.getSettings()["time"].asFloat(0.0f));
}

ScriptWaitData::ScriptWaitData(const ConfigNode& node, float time)
{
	if (node.getType() == ConfigNodeType::Map) {
		timeLeft = std::max(node["time"].asFloat(0), 0.0f);
	} else if (node.getType() != ConfigNodeType::Undefined) {
		timeLeft = std::max(node.asFloat(0), 0.0f);
	} else {
		timeLeft = std::max(time, 0.0f);
	}
}

ConfigNode ScriptWaitData::toConfigNode(const EntitySerializationContext& context)
{
	return ConfigNode(timeLeft);
}

String ScriptWait::getLabel(const BaseGraphNode& node) const
{
	if (!node.getPin(2).hasConnection()) {
		return toString(node.getSettings()["time"].asFloat(0.0f)) + "s";
	}
	return "";
}

gsl::span<const IScriptNodeType::PinType> ScriptWait::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::to_array({ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Input } });
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptWait::getSettingTypes() const
{
	return { SettingType{ "time", "float", Vector<String>{"0"} } };
}

std::pair<String, Vector<ColourOverride>> ScriptWait::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	const float time = node.getSettings()["time"].asFloat(0.0f);
	auto str = ColourStringBuilder(true);
	str.append("Wait ");
	if (node.getPin(2).hasConnection()) {
		str.append(getConnectedNodeName(node, graph, 2), parameterColour);
	} else {
		str.append(toString(time), settingColour);
	}
	str.append(time == 1.0f ? " second" : " seconds");
	return str.moveResults();
}

String ScriptWait::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 2) {
		return "Time Override";
	}
	return ScriptNodeTypeBase<ScriptWaitData>::getPinDescription(node, elementType, elementIdx);
}

IScriptNodeType::Result ScriptWait::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptWaitData& curData) const
{
	if (curData.needsCheckInput) {
		curData.needsCheckInput = false;
		auto value = readDataPin(environment, node, 2);
		if (value.getType() == ConfigNodeType::Float || value.getType() == ConfigNodeType::Int) {
			curData.timeLeft = std::max(value.asFloat(), 0.0f);
		}
	}

	const float t = static_cast<float>(time);
	const bool done = t >= curData.timeLeft;
	const float elapsed = done ? curData.timeLeft : t;
	curData.timeLeft -= elapsed;
	return Result(done ? ScriptNodeExecutionState::Done : ScriptNodeExecutionState::Executing, elapsed);
}
