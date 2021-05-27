#include "script_music.h"
using namespace Halley;

std::vector<IScriptNodeType::SettingType> ScriptPlayMusic::getSettingTypes() const
{
	return { SettingType{ "music", "Halley::String", std::vector<String>{""} } };
}

gsl::span<const IScriptNodeType::PinType> ScriptPlayMusic::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, std::vector<ColourOverride>> ScriptPlayMusic::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Play music \"");
	str.append(node.getSettings()["music"].asString(""), Colour4f(0.97f, 0.35f, 0.35f));
	str.append("\".");
	return str.moveResults();
}

IScriptNodeType::Result ScriptPlayMusic::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	environment.playMusic(node.getSettings()["music"].asString(""), 1.0f);
	return Result(ScriptNodeExecutionState::Done);
}



gsl::span<const IScriptNodeType::PinType> ScriptStopMusic::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, std::vector<ColourOverride>> ScriptStopMusic::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Stop playing music.");
	return str.moveResults();
}

IScriptNodeType::Result ScriptStopMusic::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	environment.stopMusic(1.0f);
	return Result(ScriptNodeExecutionState::Done);
}
