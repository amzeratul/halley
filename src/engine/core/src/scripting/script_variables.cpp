#include "halley/scripting/script_variables.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/bytes/config_node_serializer.h"
#include "halley/entity/entity_id.h"
#include "halley/utils/algorithm.h"

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
				context.debugCurrentContext = "ScriptVariables:" + k;
				const auto entityId = ConfigNodeSerializer<EntityId>().deserialize(context, v);
				context.debugCurrentContext = {};
				variables[k.mid(7)] = entityId;
			} else {
				variables[k] = v;
			}
		}
	} else if (node.getType() != ConfigNodeType::Undefined) {
		for (const auto& [k, v]: node.asMap()) {
			if (k.startsWith("entity!")) {
				if (v.getType() == ConfigNodeType::Del) {
					variables.erase(k.mid(7));
				} else {
					context.debugCurrentContext = "ScriptVariables:" + k;
					const auto entityId = ConfigNodeSerializer<EntityId>().deserialize(context, v);
					context.debugCurrentContext = {};
					variables[k.mid(7)] = entityId;
				}
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

ConfigNode ScriptVariables::toConfigNode() const
{
	return ConfigNode(variables);
}

ConfigNode ScriptVariables::toConfigNode(const EntitySerializationContext& context) const
{
	ConfigNode::MapType result;
	for (const auto& [k, v]: variables) {
		if (v.getType() == ConfigNodeType::EntityId) {
			result["entity!" + k] = ConfigNodeSerializer<EntityId>().serialize(v.asEntityId(), context);
		} else {
			result[k] = v;
		}
	}
	return result;
}

void ScriptVariables::serialize(Serializer& s, const EntitySerializationContext& context) const
{
	for (const auto& [k, v]: variables) {
		if (v.getType() == ConfigNodeType::EntityId) {
			// Has entity id, so we need the slow path
			s << true;
			s << toConfigNode(context);
			return;
		}
	}

	// No entity id found, just write variables
	s << false;
	s << variables;
}

void ScriptVariables::deserialize(Deserializer& s, const EntitySerializationContext& context)
{
	bool slowPath;
	s >> slowPath;
	if (slowPath) {
		ConfigNode n;
		s >> n;
		load(n, context);
		return;
	}

	s >> variables;
	Vector<std::pair<String, EntityId>> toAdd;
	for (const auto& [k, v]: variables) {
		if (k.startsWith("entity!")) {
			auto key = std::string_view(k).substr(7);
			EntityId id = ConfigNodeSerializer<EntityId>().deserialize(context, v);
			toAdd += { key, id };
		}
	}

	if (!toAdd.empty()) {
		std_ex::erase_if_key(variables, [&] (const String& key) {
			return key.startsWith("entity!");
		});
		for (auto& [k, v]: toAdd) {
			variables[k] = v;
		}
	}
}

const ConfigNode& ScriptVariables::getVariable(std::string_view name) const
{
	const auto iter = variables.find(name);
	if (iter != variables.end()) {
		return iter->second;
	}
	return dummy;
}

void ScriptVariables::setVariable(std::string_view name, ConfigNode value)
{
	variables[name] = std::move(value);
}

bool ScriptVariables::hasVariable(std::string_view name) const
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

void ByteSerializationHelper<ScriptVariables>::serialize(const ScriptVariables& value, const ByteSerializationContext& context, Serializer& serializer, int componentIndex, std::string_view fieldName)
{
	value.serialize(serializer, *context.entitySerializationContext);
}

void ByteSerializationHelper<ScriptVariables>::deserialize(ScriptVariables& dst, const ByteSerializationContext& context, Deserializer& deserializer, int componentIndex, std::string_view fieldName)
{
	dst.deserialize(deserializer, *context.entitySerializationContext);
}
