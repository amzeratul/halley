#include "script_start.h"

using namespace Halley;

IScriptNodeType::Result ScriptStart::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return { 0, ScriptNodeExecutionState::Done };
}
