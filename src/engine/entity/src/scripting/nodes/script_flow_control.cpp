#include "script_flow_control.h"

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
