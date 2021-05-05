#include "script_stop.h"
using namespace Halley;

std::pair<String, std::vector<ColourOverride>> ScriptStop::getNodeDescription(const ScriptGraphNode& node, const World& world) const
{
	return { "Terminate script.", {} };
}

IScriptNodeType::Result ScriptStop::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return { 0, ScriptNodeExecutionState::Terminate };
}
