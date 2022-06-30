#include "scripting/script_variables.h"

using namespace Halley;

ScriptVariables::ScriptVariables(const ConfigNode& node)
{
	load(node);
}

void ScriptVariables::load(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		variables = node.asHashMap<String, ConfigNode>();
	} else if (node.getType() != ConfigNodeType::Undefined) {
		for (const auto& [k, v]: node.asMap()) {
			variables[k].applyDelta(v);
		}
	}
}

ConfigNode ScriptVariables::toConfigNode() const
{
	return ConfigNode(variables);
}

const ConfigNode& ScriptVariables::getVariable(const String& name) const
{
	const auto iter = variables.find(name);
	if (iter != variables.end()) {
		return iter->second;
	}
	return dummy;
}

void ScriptVariables::setVariable(const String& name, ConfigNode value)
{
	variables[name] = std::move(value);
}

bool ScriptVariables::hasVariable(const String& name) const
{
	return variables.find(name) != variables.end();
}

bool ScriptVariables::empty() const
{
	return variables.empty();
}

ConfigNode ConfigNodeSerializer<ScriptVariables>::serialize(const ScriptVariables& variables, const EntitySerializationContext& context)
{
	return variables.toConfigNode();
}

ScriptVariables ConfigNodeSerializer<ScriptVariables>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	return ScriptVariables(node);
}

void ConfigNodeSerializer<ScriptVariables>::deserialize(const EntitySerializationContext& context, const ConfigNode& node, ScriptVariables& target)
{
	target.load(node);
}
