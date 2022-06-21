#include "scripting/script_message.h"
using namespace Halley;

ScriptMessageType::ScriptMessageType(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		script = node["script"].asString("");
		message = node["message"].asString("");
		nParams = node["nParams"].asInt(0);
	}
}

ConfigNode ScriptMessageType::toConfig() const
{
	ConfigNode::MapType result;
	result["script"] = script;
	result["message"] = message;
	result["nParams"] = nParams;
	return result;
}

ScriptMessage::ScriptMessage(const ConfigNode& node)
{
	type = node["type"];
	params = node["params"];
}

ConfigNode ScriptMessage::toConfig() const
{
	ConfigNode::MapType result;
	result["type"] = type.toConfig();
	result["params"] = params;
	return result;
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
	}
}

ConfigNode ScriptEntityMessageType::toConfig() const
{
	ConfigNode::MapType result;
	result["message"] = message;
	result["members"] = members;
	return result;
}
