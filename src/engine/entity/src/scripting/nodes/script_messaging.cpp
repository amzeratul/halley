#include "script_messaging.h"
using namespace Halley;

Vector<IScriptNodeType::SettingType> ScriptSendMessage::getSettingTypes() const
{
	return {};
}

gsl::span<const IScriptNodeType::PinType> ScriptSendMessage::getPinConfiguration(const ScriptGraphNode& node) const
{
	return {};
}

std::pair<String, Vector<ColourOverride>> ScriptSendMessage::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	return { "", {} };
}

IScriptNodeType::Result ScriptSendMessage::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Done);
}



Vector<IScriptNodeType::SettingType> ScriptReceiveMessage::getSettingTypes() const
{
	return {};
}

gsl::span<const IScriptNodeType::PinType> ScriptReceiveMessage::getPinConfiguration(const ScriptGraphNode& node) const
{
	return {};
}

std::pair<String, Vector<ColourOverride>> ScriptReceiveMessage::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	return { "", {} };
}

IScriptNodeType::Result ScriptReceiveMessage::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Done);
}



Vector<IScriptNodeType::SettingType> ScriptSendSystemMessage::getSettingTypes() const
{
	return {};
}

gsl::span<const IScriptNodeType::PinType> ScriptSendSystemMessage::getPinConfiguration(const ScriptGraphNode& node) const
{
	return {};
}

std::pair<String, Vector<ColourOverride>> ScriptSendSystemMessage::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	return { "", {} };
}

IScriptNodeType::Result ScriptSendSystemMessage::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Done);
}
