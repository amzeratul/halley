#include "script_flow_control.h"

using namespace Halley;

std::pair<String, Vector<ColourOverride>> ScriptStart::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	return { "Start execution", {} };
}

gsl::span<const IScriptNodeType::PinType> ScriptStart::getPinConfiguration(const ScriptGraphNode& node) const
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


gsl::span<const IScriptNodeType::PinType> ScriptRestart::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::FlowPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptRestart::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	return { "Restart script from beginning.", {} };
}

IScriptNodeType::Result ScriptRestart::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Restart);
}



gsl::span<const IScriptNodeType::PinType> ScriptStop::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::FlowPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptStop::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	return { "Terminate script.", {} };
}

IScriptNodeType::Result ScriptStop::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Terminate);
}
