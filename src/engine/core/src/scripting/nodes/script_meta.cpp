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



std::pair<String, Vector<ColourOverride>> ScriptDebugDisplay::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	ColourStringBuilder result;
	result.append("Display the value of ");
	result.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	return result.moveResults();
}

gsl::span<const IGraphNodeType::PinType> ScriptDebugDisplay::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::ReadDataPin, PD::Input } };
	return data;
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
	auto msg = getConnectedNodeName(world, node, graph, 2);
	if (msg == "<empty>") {
		msg = node.getSettings()["message"].asString("");
	}

	ColourStringBuilder str;
	str.append("Log ");
	str.append("\"" + msg + "\"", settingColour);
	str.append(" with severity ");
	str.append(toString(node.getSettings()["severity"].asEnum<LoggerLevel>(LoggerLevel::Dev)), settingColour);
	return str.moveResults();
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
	auto message = node.getSettings()["message"].asString("");
	const auto& data = readDataPin(environment, node, 2);
	if (data.getType() != ConfigNodeType::Undefined) {
		message += " ";
		if (data.getType() == ConfigNodeType::EntityId) {
			const auto entityId = EntityId(data.asEntityId().value);
			if (entityId.isValid()) {
				message += environment.getWorld().getEntity(entityId).getName() + " (" + toString(entityId) + ")";
			} else {
				message += "<invalid entity>";
			}
		} else {
			message += data.asString();
		}
	}

	switch(severity) {
	case LoggerLevel::Dev:
		Logger::logDev(message);
		break;

	case LoggerLevel::Info:
		Logger::logInfo(message);
		break;

	case LoggerLevel::Warning:
		Logger::logWarning(message);
		break;

	case LoggerLevel::Error:
		Logger::logError(message);
		break;
	}

	return Result(ScriptNodeExecutionState::Done);
}
