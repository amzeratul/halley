#include "script_execution_control.h"

using namespace Halley;

std::pair<String, Vector<ColourOverride>> ScriptStart::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	return { "Start script", {} };
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

std::pair<String, Vector<ColourOverride>> ScriptRestart::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	return { "Restart script", {} };
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

std::pair<String, Vector<ColourOverride>> ScriptStop::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	return { "Terminate script", {} };
}

IScriptNodeType::Result ScriptStop::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Terminate);
}



gsl::span<const IScriptNodeType::PinType> ScriptSpinwait::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::FlowPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSpinwait::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	return { "Spinwait thread", {} };
}

IScriptNodeType::Result ScriptSpinwait::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Executing, time);
}



Vector<IScriptNodeType::SettingType> ScriptStartScript::getSettingTypes() const
{
	return {
		SettingType{ "script", "Halley::ResourceReference<Halley::ScriptGraph>", Vector<String>{""} },
	};
}

std::pair<String, Vector<ColourOverride>> ScriptStartScript::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Start Script ");
	str.append(node.getSettings()["script"].asString(""), parameterColour);
	str.append(" on ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	return str.moveResults();
}

gsl::span<const IScriptNodeType::PinType> ScriptStartScript::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input } };
	return data;
}

IScriptNodeType::Result ScriptStartScript::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto& script = node.getSettings()["script"].asString("");
	const auto target = readEntityId(environment, node, 2);

	if (!script.isEmpty()) {
		// TODO
	}

	return Result(ScriptNodeExecutionState::Done);
}



Vector<IScriptNodeType::SettingType> ScriptStopScript::getSettingTypes() const
{
	return {
		SettingType{ "script", "Halley::ResourceReference<Halley::ScriptGraph>", Vector<String>{""} },
	};
}

std::pair<String, Vector<ColourOverride>> ScriptStopScript::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Stop Script ");
	str.append(node.getSettings()["script"].asString(""), parameterColour);
	str.append(" on ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	return str.moveResults();
}

gsl::span<const IScriptNodeType::PinType> ScriptStopScript::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input } };
	return data;
}

IScriptNodeType::Result ScriptStopScript::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto& script = node.getSettings()["script"].asString("");
	const auto target = readEntityId(environment, node, 2);

	if (!script.isEmpty()) {
		// TODO
	}

	return Result(ScriptNodeExecutionState::Done);
}



Vector<IScriptNodeType::SettingType> ScriptStopTag::getSettingTypes() const
{
	return {
		SettingType{ "tag", "Halley::String", Vector<String>{""} },
	};
}

std::pair<String, Vector<ColourOverride>> ScriptStopTag::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Stop all Scripts matching tag ");
	str.append(node.getSettings()["tag"].asString(""), parameterColour);
	str.append(" on ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	return str.moveResults();
}

gsl::span<const IScriptNodeType::PinType> ScriptStopTag::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input } };
	return data;
}

IScriptNodeType::Result ScriptStopTag::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto& tag = node.getSettings()["script"].asString("");
	const auto target = readEntityId(environment, node, 2);

	if (!tag.isEmpty()) {
		// TODO
	}

	return Result(ScriptNodeExecutionState::Done);
}
