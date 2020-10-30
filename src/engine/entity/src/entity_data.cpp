#include "entity_data.h"
using namespace Halley;

EntityData::EntityData()
{}

EntityData::EntityData(const ConfigNode& data)
{
	// TODO
}

ConfigNode EntityData::toConfigNode() const
{
	// TODO
	return ConfigNode();
}

void EntityData::serialize(Serializer& s) const
{
	// TODO
}

void EntityData::deserialize(Deserializer& s)
{
	// TODO
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

EntityData EntityData::makeDelta(const EntityData& to) const
{
	// TODO
	return EntityData();
}

void EntityData::applyDelta(const EntityData& delta)
{
	// TODO
}
