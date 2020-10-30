#include "entity_data.h"
using namespace Halley;

EntityData::EntityData()
{}

EntityData::EntityData(const ConfigNode& data)
{
	name = data["name"].asString("");
	prefab = data["prefab"].asString("");
	parseUUID(instanceUUID, data["instanceUUID"]);
	parseUUID(prefabUUID, data["prefabUUID"]);

	if (data.hasKey("components")) {
		for (const auto& compMap: data["components"].asSequence()) {
			for (const auto& [k, v]: compMap.asMap()) {
				addComponent(k, ConfigNode(v));
			}
		}
	}

	if (data.hasKey("children")) {
		for (const auto& childNode: data["children"].asSequence()) {
			children.emplace_back(childNode);
		}
	}
}

ConfigNode EntityData::toConfigNode() const
{
	ConfigNode::MapType result;

	if (!name.isEmpty()) {
		result["name"] = name;
	}
	if (!prefab.isEmpty()) {
		result["prefab"] = prefab;
	}
	if (instanceUUID.isValid()) {
		result["instanceUUID"] = instanceUUID.toString();
	}
	if (prefabUUID.isValid()) {
		result["uuid"] = prefabUUID.toString();
	}

	if (!components.empty()) {
		ConfigNode::SequenceType compNodes;
		for (const auto& comp: components) {
			ConfigNode::MapType entry;
			entry[comp.first] = ConfigNode(comp.second);
			compNodes.emplace_back(std::move(entry));
		}
		result["components"] = std::move(compNodes);
	}

	if (!children.empty()) {
		ConfigNode::SequenceType childNodes;
		for (const auto& child: children) {
			childNodes.emplace_back(child.toConfigNode());
		}
		result["children"] = std::move(childNodes);
	}
	
	return ConfigNode(std::move(result));
}

void EntityData::serialize(Serializer& s) const
{
	// TODO
	throw Exception("Unimplemented", HalleyExceptions::Entity);
}

void EntityData::deserialize(Deserializer& s)
{
	// TODO
	throw Exception("Unimplemented", HalleyExceptions::Entity);
}

void EntityData::setName(String name)
{
	this->name = std::move(name);
}

void EntityData::setPrefab(String prefab)
{
	this->prefab = std::move(prefab);
}

void EntityData::setInstanceUUID(UUID instanceUUID)
{
	this->instanceUUID = std::move(instanceUUID);
}

void EntityData::setPrefabUUID(UUID prefabUUID)
{
	this->prefabUUID = std::move(prefabUUID);
}

void EntityData::setChildren(std::vector<EntityData> children)
{
	this->children = std::move(children);
}

void EntityData::setComponents(std::vector<std::pair<String, ConfigNode>> components)
{
	this->components = std::move(components);
}

EntityData EntityData::makeDelta(const EntityData& from, const EntityData& to)
{
	EntityData delta;

	if (from.name != to.name) {
		delta.name = to.name;
		delta.setFieldPresent(FieldId::Name, true);
	}
	if (from.prefab != to.prefab) {
		delta.prefab = to.prefab;
		delta.setFieldPresent(FieldId::Prefab, true);
	}
	if (from.instanceUUID != to.instanceUUID) {
		delta.instanceUUID = to.instanceUUID;
		delta.setFieldPresent(FieldId::InstanceUUID, true);
	}
	if (from.prefabUUID != to.prefabUUID) {
		delta.prefabUUID = to.prefabUUID;
		delta.setFieldPresent(FieldId::PrefabUUID, true);
	}

	return delta;
}

void EntityData::applyDelta(const EntityData& delta)
{
	if (delta.isFieldPresent(FieldId::Name)) {
		name = delta.name;
	}
	if (delta.isFieldPresent(FieldId::Prefab)) {
		prefab = delta.prefab;
	}
	if (delta.isFieldPresent(FieldId::InstanceUUID)) {
		instanceUUID = delta.instanceUUID;
	}
	if (delta.isFieldPresent(FieldId::PrefabUUID)) {
		prefabUUID = delta.prefabUUID;
	}
	
}

void EntityData::addComponent(String key, ConfigNode data)
{
	components.emplace_back(std::move(key), std::move(data));
}

void EntityData::parseUUID(UUID& dst, const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Bytes) {
		dst = UUID(node.asBytes());
	} else if (node.getType() == ConfigNodeType::String) {
		dst = UUID(node.asString());
	}
}

uint8_t EntityData::getFieldBit(FieldId id) const
{
	return static_cast<uint8_t>(1 << static_cast<int>(id));
}

void EntityData::setFieldPresent(FieldId id, bool present)
{
	if (present) {
		fieldPresent |= getFieldBit(id);
	} else {
		fieldPresent &= ~getFieldBit(id);
	}
}

bool EntityData::isFieldPresent(FieldId id) const
{
	return (fieldPresent & getFieldBit(id)) != 0;
}
