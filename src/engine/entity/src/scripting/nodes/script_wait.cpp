#include "script_wait.h"
using namespace Halley;

void ScriptWait::doInitData(ScriptWaitData& data, const ScriptGraphNode& node) const
{
	data.timeLeft = static_cast<Time>(node.getSettings()["time"].asFloat(0.0f));
}

std::pair<String, std::vector<ColourOverride>> ScriptWait::getDescription(const ScriptGraphNode& node, const World& world) const
{
	String text;
	std::vector<ColourOverride> cols;
	const float time = node.getSettings()["time"].asFloat(0.0f);

	text += "Wait ";
	cols.emplace_back(text.length(), Colour4f(0.97f, 0.35f, 0.35f));
	text += toString(time);
	cols.emplace_back(text.length(), std::optional<Colour4f>());
	if (time == 1.0f) {
		text += " second.";
	} else {
		text += " seconds.";
	}

	return { std::move(text), std::move(cols) };
}

IScriptNodeType::Result ScriptWait::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptWaitData& curData) const
{
	const bool done = time >= curData.timeLeft;
	const Time elapsed = done ? curData.timeLeft : time;
	curData.timeLeft -= elapsed;
	return { elapsed, done ? ScriptNodeExecutionState::Done : ScriptNodeExecutionState::Executing };
}
