#include "halley/scripting/script_variables.h"

#include "halley/entity/entity_id.h"

using namespace Halley;

ScriptVariables::ScriptVariables(const ConfigNode& node, const EntitySerializationContext& context)
{
	load(node, context);
}

void ScriptVariables::load(const ConfigNode& node, const EntitySerializationContext& context)
{
	if (node.getType() == ConfigNodeType::Map) {
		variables.clear();
		for (const auto& [k, v]: node.asMap()) {
			if (k.startsWith("entity!")) {
				const auto entityId = ConfigNodeSerializer<EntityId>().deserialize(context, v);
				variables[k.mid(7)] = EntityIdHolder{ entityId.value };
			} else {
				variables[k] = v;
			}
		}
	} else if (node.getType() != ConfigNodeType::Undefined) {
		for (const auto& [k, v]: node.asMap()) {
			if (k.startsWith("entity!")) {
				const auto entityId = ConfigNodeSerializer<EntityId>().deserialize(context, v);
				variables[k.mid(7)] = EntityIdHolder{ entityId.value };
			} else {
				if (v.getType() == ConfigNodeType::Del) {
					variables.erase(k);
				} else {
					variables[k].applyDelta(v);
				}
			}
		}
	}
}

ConfigNode ScriptVariables::toConfigNode(const EntitySerializationContext& context) const
{
	ConfigNode::MapType result;
	for (const auto& [k, v]: variables) {
		if (v.getType() == ConfigNodeType::EntityId) {
			result["entity!" + k] = ConfigNodeSerializer<EntityId>().serialize(EntityId(v.asEntityId().value), context);
		} else {
			result[k] = v;
		}
	}
	return result;
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

void ScriptVariables::clear()
{
	variables.clear();
}

ConfigNode ConfigNodeSerializer<ScriptVariables>::serialize(const ScriptVariables& variables, const EntitySerializationContext& context)
{
	return variables.toConfigNode(context);
}

ScriptVariables ConfigNodeSerializer<ScriptVariables>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	return ScriptVariables(node, context);
}

void ConfigNodeSerializer<ScriptVariables>::deserialize(const EntitySerializationContext& context, const ConfigNode& node, ScriptVariables& target)
{
	target.load(node, context);
}
