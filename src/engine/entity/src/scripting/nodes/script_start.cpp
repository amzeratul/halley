#include "script_start.h"

using namespace Halley;

std::pair<String, std::vector<ColourOverride>> ScriptStart::getNodeDescription(const ScriptGraphNode& node, const World& world) const
{
	return { "Start execution", {} };
}

IScriptNodeType::Result ScriptStart::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Done);
}
