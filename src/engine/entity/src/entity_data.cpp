#include "entity_data.h"
#include "entity_data_delta.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/support/logger.h"
using namespace Halley;

EntityData::EntityData()
{}

EntityData::EntityData(UUID instanceUUID)
	: instanceUUID(instanceUUID)
{
}

EntityData::EntityData(const ConfigNode& data, bool isPrefab)
	: fromPrefab(isPrefab)
{
	name = data["name"].asString("");
	prefab = data["prefab"].asString("");

	if (isPrefab) {
		parseUUID(prefabUUID, data["uuid"]);
		if (!prefabUUID.isValid()) {
			prefabUUID = UUID::generate();
		}
		instanceUUID = prefabUUID;
	} else {
		parseUUID(instanceUUID, data["uuid"]);
		parseUUID(prefabUUID, data["prefabUUID"]);

		if (!instanceUUID.isValid()) {
			instanceUUID = UUID::generate();
		}
	}
	parseUUID(parentUUID, data["parent"]);

	if (data.hasKey("components")) {
		for (const auto& compMap: data["components"].asSequence()) {
			for (const auto& [k, v]: compMap.asMap()) {
				addComponent(k, ConfigNode(v));
			}
		}
	}

	if (data.hasKey("children")) {
		for (const auto& childNode: data["children"].asSequence()) {
			children.emplace_back(childNode, isPrefab);
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
		result["uuid"] = instanceUUID.toString();
	}
	if (prefabUUID.isValid()) {
		result["prefabUUID"] = prefabUUID.toString();
	}
	if (parentUUID.isValid()) {
		result["parent"] = parentUUID.toString();
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

String EntityData::toYAML() const
{
	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = {{ "name", "uuid", "prefabUUID", "parent", "components", "children" }};
	return YAMLConvert::generateYAML(toConfigNode(), options);
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

const EntityData* EntityData::tryGetPrefabUUID(const UUID& uuid) const
{
	Expects(uuid.isValid());
	
	if (uuid == prefabUUID) {
		return this;
	}

	for (const auto& c: children) {
		const auto* result = c.tryGetPrefabUUID(uuid);
		if (result) {
			return result;
		}
	}

	return nullptr;
}

const EntityData* EntityData::tryGetInstanceUUID(const UUID& uuid) const
{
	Expects(uuid.isValid());
	
	if (uuid == instanceUUID) {
		return this;
	}

	for (const auto& c: children) {
		const auto* result = c.tryGetInstanceUUID(uuid);
		if (result) {
			return result;
		}
	}

	return nullptr;
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

void EntityData::setParentUUID(UUID parentUUID)
{
	this->parentUUID = std::move(parentUUID);
}

void EntityData::setChildren(std::vector<EntityData> children)
{
	this->children = std::move(children);
}

void EntityData::setComponents(std::vector<std::pair<String, ConfigNode>> components)
{
	this->components = std::move(components);
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

void EntityData::applyDelta(const EntityDataDelta& delta)
{
	if (delta.name) {
		name = delta.name.value();
	}
	if (delta.prefab) {
		prefab = delta.prefab.value();
	}
	if (delta.instanceUUID) {
		instanceUUID = delta.instanceUUID.value();
	}
	if (delta.prefabUUID) {
		prefabUUID = delta.prefabUUID.value();
	}
	if (delta.parentUUID) {
		parentUUID = delta.parentUUID.value();
	}
	
	for (const auto& childId: delta.childrenRemoved) {
		children.erase(std::remove_if(children.begin(), children.end(), [&] (const auto& child) { return child.getPrefabUUID() == childId; }), children.end());
	}
	for (const auto& child: delta.childrenChanged) {
		auto iter = std::find_if(children.begin(), children.end(), [&] (const auto& cur) { return cur.getPrefabUUID() == child.first; });
		if (iter == children.end()) {
			children.emplace_back(applyDelta(EntityData(child.first), child.second));
		} else {
			iter->applyDelta(child.second);
		}
	}
	if (!delta.childrenOrder.empty()) {
		// TODO
	}
	
	for (const auto& componentId: delta.componentsRemoved) {
		components.erase(std::remove_if(components.begin(), components.end(), [&] (const auto& component) { return component.first == componentId; }), components.end());
	}
	for (const auto& component: delta.componentsChanged) {
		auto iter = std::find_if(components.begin(), components.end(), [&] (const auto& cur) { return cur.first == component.first; });
		if (iter == components.end()) {
			components.emplace_back(component.first, ConfigNode::applyDelta(ConfigNode(ConfigNode::MapType()), component.second));
		} else {
			iter->second.applyDelta(component.second);
		}
	}
	if (!delta.componentOrder.empty()) {
		// TODO
	}
}

EntityData EntityData::applyDelta(EntityData src, const EntityDataDelta& delta)
{
	src.applyDelta(delta);
	return src;
}

bool EntityData::isSameEntity(const EntityData& other) const
{
	return prefabUUID == other.prefabUUID;
}

void EntityData::instantiateWith(const EntityData& instance)
{
	// This should only be called on the root of prefab
	Expects(fromPrefab);
	Expects(instance.instanceUUID.isValid());
	
	instanceUUID = instance.instanceUUID;

	// Update children UUIDs
	for (auto& c: children) {
		c.generateChildUUID(instanceUUID);
	}

	// Update components and children
	instantiateData(instance);
}

void EntityData::generateChildUUID(const UUID& root)
{
	instanceUUID = UUID::generateFromUUIDs(prefabUUID, root);

	for (auto& c: children) {
		c.generateChildUUID(root);
	}
}

void EntityData::instantiateData(const EntityData& instance)
{
	for (const auto& c: instance.components) {
		updateComponent(c.first, c.second);
	}
	for (const auto& c: instance.children) {
		updateChild(c);
	}
}

void EntityData::updateComponent(const String& id, const ConfigNode& data)
{
	for (auto& c: components) {
		if (c.first == id) {
			c.second = ConfigNode(data);
			return;
		}
	}
	components.emplace_back(id, ConfigNode(data));
}

void EntityData::updateChild(const EntityData& instanceChildData)
{
	for (auto& c: children) {
		// Is this correct???
		Logger::logWarning("Untested code");
		if (c.prefabUUID == instanceChildData.prefabUUID) {
			c.instantiateData(instanceChildData);
			return;
		}
	}
	children.emplace_back(instanceChildData);
}

EntityData EntityData::instantiateWithAsCopy(const EntityData& instance) const
{
	EntityData clone = *this;
	clone.instantiateWith(instance);
	return clone;
}
