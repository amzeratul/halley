#include "script_start.h"

using namespace Halley;

std::pair<String, std::vector<ColourOverride>> ScriptStart::getNodeDescription(const ScriptGraphNode& node, const World& world) const
{
	return { "Start execution", {} };
}

gsl::span<const IScriptNodeType::PinType> ScriptStart::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::FlowPin, PD::Output } };
	return data;
}

IScriptNodeType::Result ScriptStart::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Done);
}
