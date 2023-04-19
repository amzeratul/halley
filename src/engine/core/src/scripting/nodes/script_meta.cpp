#include "script_meta.h"

using namespace Halley;

Vector<IGraphNodeType::SettingType> ScriptComment::getSettingTypes() const
{
	return {
		{ "comment", "Halley::String", {} }
	};
}

std::pair<String, Vector<ColourOverride>> ScriptComment::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	return { "Comment", {} };
}

gsl::span<const IGraphNodeType::PinType> ScriptComment::getPinConfiguration(const ScriptGraphNode& node) const
{
	return {};
}

String ScriptComment::getLargeLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["comment"].asString("");
}


Vector<IGraphNodeType::SettingType> ScriptLog::getSettingTypes() const
{
	return {
		{ "severity", "Halley::LoggerLevel", Vector<String>{"dev"} },
		{ "message", "Halley::String", {} }
	};
}

std::pair<String, Vector<ColourOverride>> ScriptLog::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	return { "Log", {} };
}

gsl::span<const IGraphNodeType::PinType> ScriptLog::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Input } };
	return data;
}

String ScriptLog::getLargeLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["severity"].asString("");
}

IScriptNodeType::Result ScriptLog::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto& severity = node.getSettings()["severity"].asEnum<LoggerLevel>(LoggerLevel::Dev);
	const auto& message = node.getSettings()["message"].asString("");
	const auto& data = readDataPin(environment, node, 2);

	switch(severity) {
	case LoggerLevel::Dev:
		Logger::logDev(message + data.asString());
		break;

	case LoggerLevel::Info:
		Logger::logInfo(message + data.asString());
		break;

	case LoggerLevel::Warning:
		Logger::logWarning(message + data.asString());
		break;

	case LoggerLevel::Error:
		Logger::logError(message + data.asString());
		break;
	}

	return Result(ScriptNodeExecutionState::Done);
}
