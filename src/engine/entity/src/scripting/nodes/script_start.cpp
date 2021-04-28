#include "script_start.h"

using namespace Halley;

std::vector<IScriptNodeType::SettingType> ScriptStart::getSettingTypes() const
{
	return {};
}

std::pair<String, std::vector<ColourOverride>> ScriptStart::getDescription(const ScriptGraphNode& node, const World& world) const
{
	return { "Start execution", {} };
}

IScriptNodeType::Result ScriptStart::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return { 0, ScriptNodeExecutionState::Done };
}
