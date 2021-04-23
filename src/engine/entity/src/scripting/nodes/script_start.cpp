#include "script_start.h"

using namespace Halley;

std::pair<String, std::vector<ColourOverride>> ScriptStart::getDescription(const ScriptGraphNode& node) const
{
	return { "Start execution", {} };
}

IScriptNodeType::Result ScriptStart::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return { 0, ScriptNodeExecutionState::Done };
}
