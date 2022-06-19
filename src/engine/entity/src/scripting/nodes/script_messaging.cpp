#include "script_messaging.h"

#include "halley/support/logger.h"
using namespace Halley;

Vector<IScriptNodeType::SettingType> ScriptSendMessage::getSettingTypes() const
{
	return {
		SettingType{ "script", "Halley::ResourceReference<Halley::ScriptGraph>", Vector<String>{""} },
		SettingType{ "message", "Halley::String", Vector<String>{""} },
		SettingType{ "parameters", "Halley::Range<int, 0, 4>", Vector<String>{"0"} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptSendMessage::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 7>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input } };

	const int nArgs = clamp(node.getSettings()["parameters"].asInt(0), 0, 4);

	return gsl::span<const IScriptNodeType::PinType>(data).subspan(0, 3 + nArgs);
}

std::pair<String, Vector<ColourOverride>> ScriptSendMessage::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Send message ");
	str.append(node.getSettings()["message"].asString(""), parameterColour);
	str.append("(");

	const int nArgs = clamp(node.getSettings()["parameters"].asInt(0), 0, 4);
	for (int i = 0; i < nArgs; ++i) {
		if (i != 0) {
			str.append(", ");
		}
		str.append(getConnectedNodeName(world, node, graph, 3 + i), parameterColour);
	}

	str.append(") to script ");
	str.append(node.getSettings()["script"].asString(""), parameterColour);
	str.append(" on entity ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	return str.moveResults();
}

std::pair<String, Vector<ColourOverride>> ScriptSendMessage::getPinDescription(const ScriptGraphNode& node, PinType elementType, ScriptPinId elementIdx) const
{
	if (elementIdx >= 3) {
		return { "Message parameter #" + toString(elementIdx - 2), {}};
	} else {
		return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
	}
}

IScriptNodeType::Result ScriptSendMessage::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto& msg = node.getSettings()["message"].asString("");
	const auto& script = node.getSettings()["script"].asString("");
	const auto entityId = readEntityId(environment, node, 2);
	const auto& entityName = environment.tryGetEntity(entityId).getName();

	Logger::logDev("Sending message " + script + ":" + msg + " to " + entityName);
	return Result(ScriptNodeExecutionState::Done);
}



Vector<IScriptNodeType::SettingType> ScriptReceiveMessage::getSettingTypes() const
{
	return {
		SettingType{ "message", "Halley::String", Vector<String>{""} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptReceiveMessage::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptReceiveMessage::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("On receive message ");
	str.append(node.getSettings()["message"].asString(""), parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptReceiveMessage::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Done);
}



Vector<IScriptNodeType::SettingType> ScriptSendSystemMessage::getSettingTypes() const
{
	return {
		SettingType{ "system", "Halley::String", Vector<String>{""} },
		SettingType{ "message", "Halley::String", Vector<String>{""} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptSendSystemMessage::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSendSystemMessage::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Send message ");
	str.append(node.getSettings()["message"].asString(""), parameterColour);
	str.append(" to system ");
	str.append(node.getSettings()["system"].asString(""), parameterColour);
	return str.moveResults();}

IScriptNodeType::Result ScriptSendSystemMessage::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	// TODO
	return Result(ScriptNodeExecutionState::Done);
}
