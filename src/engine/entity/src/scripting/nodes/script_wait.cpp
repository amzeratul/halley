#include "script_wait.h"
using namespace Halley;

void ScriptWait::doInitData(ScriptWaitData& data, const ConfigNode& settings)
{
	data.timeLeft = static_cast<Time>(settings["time"].asFloat(0.0f));
}

IScriptNodeType::Result ScriptWait::doUpdate(ScriptEnvironment& environment, Time time, const ConfigNode& settings,	ScriptWaitData& curData)
{
	const bool done = time >= curData.timeLeft;
	const Time elapsed = done ? curData.timeLeft : time;
	curData.timeLeft -= elapsed;
	return { elapsed, done };
}
