#include "script_messaging.h"

#include "halley/support/logger.h"
#include "scripting/script_message.h"
using namespace Halley;

Vector<IScriptNodeType::SettingType> ScriptSendMessage::getSettingTypes() const
{
	return {
		SettingType{ "message", "Halley::ScriptMessageType", Vector<String>{""} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptSendMessage::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 7>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input } };

	const auto msgType = ScriptMessageType(node.getSettings()["message"]);

	return gsl::span<const IScriptNodeType::PinType>(data).subspan(0, 3 + msgType.nParams);
}

std::pair<String, Vector<ColourOverride>> ScriptSendMessage::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	const auto msgType = ScriptMessageType(node.getSettings()["message"]);

	auto str = ColourStringBuilder(true);
	str.append("Send message ");
	str.append(msgType.message, parameterColour);
	str.append("(");

	for (int i = 0; i < msgType.nParams; ++i) {
		if (i != 0) {
			str.append(", ");
		}
		str.append(getConnectedNodeName(world, node, graph, 3 + i), parameterColour);
	}

	str.append(") to script ");
	str.append(msgType.script, parameterColour);
	str.append(" on entity ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	return str.moveResults();
}

std::pair<String, Vector<ColourOverride>> ScriptSendMessage::getPinDescription(const ScriptGraphNode& node, PinType elementType, ScriptPinId elementIdx) const
{
	if (elementIdx >= 3) {
		return { "Parameter #" + toString(elementIdx - 2), {}};
	} else {
		return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
	}
}

IScriptNodeType::Result ScriptSendMessage::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	ScriptMessage msg;
	ScriptMessageType& msgType = msg.type;
	msgType = ScriptMessageType(node.getSettings()["message"]);

	if (msgType.nParams > 0) {
		msg.params = ConfigNode::SequenceType();
		for (int i = 0; i < msgType.nParams; ++i) {
			msg.params.asSequence().push_back(readDataPin(environment, node, 3 + i));
		}
	}

	const auto entityId = readEntityId(environment, node, 2);
	environment.sendMessage(entityId, std::move(msg));
	
	return Result(ScriptNodeExecutionState::Done);
}

ConfigNode ScriptReceiveMessageData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["curArgs"] = curArgs;
	result["hasMessageActive"] = hasMessageActive;
	return result;
}


Vector<IScriptNodeType::SettingType> ScriptReceiveMessage::getSettingTypes() const
{
	return {
		SettingType{ "message", "Halley::String", Vector<String>{""} },
		SettingType{ "nParams", "Halley::Range<int, 0, 4>", Vector<String>{"0"} },
		SettingType{ "allowSpawning", "bool", Vector<String>{"false"} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptReceiveMessage::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 5>{ PinType{ ET::FlowPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output } };
	const int nParams = node.getSettings()["nParams"].asInt(0);

	return gsl::span<const PinType>(data).subspan(0, 1 + nParams);
}

std::pair<String, Vector<ColourOverride>> ScriptReceiveMessage::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("On receive message ");
	str.append(node.getSettings()["message"].asString(""), parameterColour);
	return str.moveResults();
}

std::pair<String, Vector<ColourOverride>> ScriptReceiveMessage::getPinDescription(const ScriptGraphNode& node, PinType element, ScriptPinId elementIdx) const
{
	if (elementIdx >= 1) {
		return { "Parameter #" + toString(static_cast<int>(elementIdx)), {} };
	} else {
		return ScriptNodeTypeBase<ScriptReceiveMessageData>::getPinDescription(node, element, elementIdx);
	}
}

bool ScriptReceiveMessage::hasDestructor() const
{
	return true;
}

bool ScriptReceiveMessage::showDestructor() const
{
	return false;
}

void ScriptReceiveMessage::doInitData(ScriptReceiveMessageData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	if (nodeData.getType() != ConfigNodeType::Undefined) {
		data.curArgs = nodeData["curArgs"];
		data.hasMessageActive = nodeData["hasMessageActive"].asBool(false);
	}
}

IScriptNodeType::Result ScriptReceiveMessage::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptReceiveMessageData& data) const
{
	return Result(ScriptNodeExecutionState::Done);
}

ConfigNode ScriptReceiveMessage::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptReceiveMessageData& data) const
{
	const size_t argN = pinN - 1;
	if (!data.hasMessageActive || data.curArgs.getType() == ConfigNodeType::Undefined || argN >= data.curArgs.asSequence().size()) {
		return ConfigNode();
	}
	return ConfigNode(data.curArgs.asSequence()[argN]);
}

void ScriptReceiveMessage::doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptReceiveMessageData& data) const
{
	data.hasMessageActive = false;
	data.curArgs = ConfigNode();
}

bool ScriptReceiveMessage::canReceiveMessage(const ScriptGraphNode& node, const String& messageId, bool requiresSpawningScript) const
{
	if (messageId != node.getSettings()["message"].asString("")) {
		return false;
	}

	if (requiresSpawningScript && !node.getSettings()["allowSpawning"].asBool(false)) {
		return false;
	}

	return true;
}

bool ScriptReceiveMessage::tryReceiveMessage(const ScriptGraphNode& node, ScriptReceiveMessageData& data, ScriptMessage& msg) const
{
	Expects(msg.type.message == node.getSettings()["message"].asString(""));

	if (data.hasMessageActive) {
		return false;
	}

	data.hasMessageActive = true;
	data.curArgs = std::move(msg.params);
	return true;
}

std::pair<String, int> ScriptReceiveMessage::getMessageIdAndParams(const ScriptGraphNode& node) const
{
	return { node.getSettings()["message"].asString(""), node.getSettings()["nParams"].asInt(0) };
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



Vector<IScriptNodeType::SettingType> ScriptSendEntityMessage::getSettingTypes() const
{
	return {
		SettingType{ "message", "Halley::EntityMessageType", Vector<String>{""} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptSendEntityMessage::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSendEntityMessage::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	const auto msgType = ScriptEntityMessageType(node.getSettings()["message"]);
	
	auto str = ColourStringBuilder(true);
	str.append("Send message ");
	str.append(msgType.message, parameterColour);
	str.append(" to entity ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptSendEntityMessage::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto msgType = ScriptEntityMessageType(node.getSettings()["message"]);
	
	// TODO
	return Result(ScriptNodeExecutionState::Done);
}
