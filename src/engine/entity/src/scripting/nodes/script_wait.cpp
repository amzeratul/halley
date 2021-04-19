#include "script_wait.h"
using namespace Halley;

void ScriptWait::doInitData(ScriptWaitData& data, const ScriptGraphNode& node)
{
	data.timeLeft = static_cast<Time>(node.getSettings()["time"].asFloat(0.0f));
}

IScriptNodeType::Result ScriptWait::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptWaitData& curData)
{
	const bool done = time >= curData.timeLeft;
	const Time elapsed = done ? curData.timeLeft : time;
	curData.timeLeft -= elapsed;
	return { elapsed, done ? ScriptNodeExecutionState::Done : ScriptNodeExecutionState::Executing };
}
