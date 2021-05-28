#include "script_wait.h"
using namespace Halley;

void ScriptWait::doInitData(ScriptWaitData& data, const ScriptGraphNode& node, const ConfigNode& nodeData) const
{
	if (nodeData.getType() == ConfigNodeType::Undefined) {
		data.timeLeft = node.getSettings()["time"].asFloat(0.0f);
	} else {
		data.timeLeft = nodeData["time"].asFloat(0);
	}
}

ConfigNode ScriptWaitData::toConfigNode(const ConfigNodeSerializationContext& context)
{
	ConfigNode::MapType node;
	node["time"] = timeLeft;
	return node;
}

String ScriptWait::getLabel(const ScriptGraphNode& node) const
{
	return toString(node.getSettings()["time"].asFloat(0.0f)) + "s";
}

gsl::span<const IScriptNodeType::PinType> ScriptWait::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::vector<IScriptNodeType::SettingType> ScriptWait::getSettingTypes() const
{
	return { SettingType{ "time", "float", std::vector<String>{"0"} } };
}

std::pair<String, std::vector<ColourOverride>> ScriptWait::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	const float time = node.getSettings()["time"].asFloat(0.0f);
	ColourStringBuilder str;
	str.append("Wait ");
	str.append(toString(time), Colour4f(0.97f, 0.35f, 0.35f));
	str.append(time == 1.0f ? " second" : " seconds");
	return str.moveResults();
}

IScriptNodeType::Result ScriptWait::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptWaitData& curData) const
{
	const float t = static_cast<float>(time);
	const bool done = t >= curData.timeLeft;
	const float elapsed = done ? curData.timeLeft : t;
	curData.timeLeft -= elapsed;
	return Result(done ? ScriptNodeExecutionState::Done : ScriptNodeExecutionState::Executing, elapsed);
}
