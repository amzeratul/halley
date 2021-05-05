#include "script_restart.h"
using namespace Halley;

std::pair<String, std::vector<ColourOverride>> ScriptRestart::getNodeDescription(const ScriptGraphNode& node, const World& world) const
{
	return { "Restart script from beginning.", {} };
}

IScriptNodeType::Result ScriptRestart::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return { 0, ScriptNodeExecutionState::Restart };
}
