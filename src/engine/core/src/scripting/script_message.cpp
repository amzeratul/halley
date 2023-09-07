#include "halley/scripting/script_message.h"
#include "halley/bytes/byte_serializer.h"
using namespace Halley;

ScriptMessageType::ScriptMessageType(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		script = node["script"].asString("");
		message = node["message"].asString("");
		nParams = node["nParams"].asInt(0);
	}
}

ScriptMessageType::ScriptMessageType(String script, String message, int nParams)
	: script(std::move(script))
	, message(std::move(message))
	, nParams(nParams)
{
}

ConfigNode ScriptMessageType::toConfig() const
{
	ConfigNode::MapType result;
	result["script"] = script;
	result["message"] = message;
	result["nParams"] = nParams;
	return result;
}

void ScriptMessageType::serialize(Serializer& s) const
{
	s << script;
	s << message;
	s << nParams;
}

void ScriptMessageType::deserialize(Deserializer& s)
{
	s >> script;
	s >> message;
	s >> nParams;
}

ScriptMessage::ScriptMessage(String script, String message)
{
	type = ScriptMessageType();
	type.message = std::move(message);
	type.script = std::move(script);
}

ScriptMessage::ScriptMessage(String script, String message, ConfigNode params)
{
	type = ScriptMessageType();
	type.message = std::move(message);
	type.script = std::move(script);

	this->params = std::move(params);
}

ScriptMessage::ScriptMessage(const ConfigNode& node)
{
	type = node["type"];
	params = node["params"];
	delay = node["delay"].asFloat(0);
}

ConfigNode ScriptMessage::toConfig() const
{
	ConfigNode::MapType result;
	result["type"] = type.toConfig();
	result["params"] = params;
	result["delay"] = delay;
	return result;
}

void ScriptMessage::serialize(Serializer& s) const
{
	s << type;
	s << params;
	s << delay;
}

void ScriptMessage::deserialize(Deserializer& s)
{
	s >> type;
	s >> params;
	s >> delay;
}

String ScriptMessage::toString() const
{
	return type.message + "(" + params.asString() + ")";
}


ScriptEntityMessageType::ScriptEntityMessageType(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		message = node["message"].asString("");
		members = node["members"].asVector<String>({});
		returnType = node["returnType"].asString("");
	}
}

ConfigNode ScriptEntityMessageType::toConfig() const
{
	ConfigNode::MapType result;
	result["message"] = message;
	result["members"] = members;
	result["returnType"] = returnType;
	return result;
}

ConfigNode ConfigNodeSerializer<ScriptMessage>::serialize(const ScriptMessage& msg, const EntitySerializationContext& context)
{
	return msg.toConfig();
}

ScriptMessage ConfigNodeSerializer<ScriptMessage>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	return ScriptMessage(node);
}
