#include "script_wait.h"
using namespace Halley;

void ScriptWait::doInitData(ScriptWaitData& data, const ScriptGraphNode& node) const
{
	data.timeLeft = static_cast<Time>(node.getSettings()["time"].asFloat(0.0f));
}

std::vector<IScriptNodeType::SettingType> ScriptWait::getSettingTypes() const
{
	return { SettingType{ "time", "float", std::vector<String>{"0"} } };
}

std::pair<String, std::vector<ColourOverride>> ScriptWait::getNodeDescription(const ScriptGraphNode& node, const World& world) const
{
	const float time = node.getSettings()["time"].asFloat(0.0f);
	ColourStringBuilder str;
	str.append("Wait ");
	str.append(toString(time), Colour4f(0.97f, 0.35f, 0.35f));
	str.append(time == 1.0f ? " second." : " seconds.");
	return str.moveResults();
}

IScriptNodeType::Result ScriptWait::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptWaitData& curData) const
{
	const bool done = time >= curData.timeLeft;
	const Time elapsed = done ? curData.timeLeft : time;
	curData.timeLeft -= elapsed;
	return Result(done ? ScriptNodeExecutionState::Done : ScriptNodeExecutionState::Executing, elapsed);
}
