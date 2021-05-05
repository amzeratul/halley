#include "script_stop.h"
using namespace Halley;

gsl::span<const IScriptNodeType::PinType> ScriptStop::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::FlowPin, PD::Input } };
	return data;
}

std::pair<String, std::vector<ColourOverride>> ScriptStop::getNodeDescription(const ScriptGraphNode& node, const World& world) const
{
	return { "Terminate script.", {} };
}

IScriptNodeType::Result ScriptStop::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Terminate);
}
