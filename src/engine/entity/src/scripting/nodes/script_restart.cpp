#include "script_restart.h"
using namespace Halley;

gsl::span<const IScriptNodeType::PinType> ScriptRestart::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::FlowPin, PD::Input } };
	return data;
}

std::pair<String, std::vector<ColourOverride>> ScriptRestart::getNodeDescription(const ScriptGraphNode& node, const World& world) const
{
	return { "Restart script from beginning.", {} };
}

IScriptNodeType::Result ScriptRestart::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Restart);
}
