#include "entity_data.h"
#include "entity_data_delta.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
using namespace Halley;

EntityChangeOperation EntityChangeOperation::clone() const
{
	EntityChangeOperation result;
	result.entityId = entityId;
	result.parent = parent;
	result.childIndex = childIndex;

	if (data->isDelta()) {
		result.data = std::make_unique<EntityDataDelta>(data->asEntityDataDelta());
	} else {
		result.data = std::make_unique<EntityData>(data->asEntityData());
	}

	return result;
}

EntityData::EntityData()
{}

EntityData::EntityData(UUID instanceUUID)
	: instanceUUID(instanceUUID)
{
}

EntityData::EntityData(const ConfigNode& data, bool isPrefab)
{
	name = data["name"].asString("");
	prefab = data["prefab"].asString("");
	icon = data["icon"].asString("");

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

EntityData::EntityData(const EntityDataDelta& delta)
{
	applyDelta(delta);
}

ConfigNode EntityData::toConfigNode(bool allowPrefabUUID) const
{
	ConfigNode::MapType result;

	if (!name.isEmpty()) {
		result["name"] = name;
	}
	if (!prefab.isEmpty()) {
		result["prefab"] = prefab;
	}
	if (!icon.isEmpty()) {
		result["icon"] = icon;
	}
	if (instanceUUID.isValid()) {
		result["uuid"] = instanceUUID.toString();
	}
	if (allowPrefabUUID && prefabUUID.isValid()) {
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
			childNodes.emplace_back(child.toConfigNode(allowPrefabUUID));
		}
		result["children"] = std::move(childNodes);
	}
	
	return ConfigNode(std::move(result));
}

String EntityData::toYAML() const
{
	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = {{ "name", "prefab", "icon", "uuid", "prefabUUID", "parent", "components", "children" }};
	return YAMLConvert::generateYAML(toConfigNode(true), options);
}

void EntityData::serialize(Serializer& s) const
{
	s << EntityDataDelta(*this);
}

void EntityData::deserialize(Deserializer& s)
{
	EntityDataDelta delta;
	s >> delta;
	applyDelta(delta);
}

bool EntityData::hasComponent(const String& componentName) const
{
	for (const auto& c: components) {
		if (c.first == componentName) {
			return true;
		}
	}
	return false;
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

EntityData* EntityData::tryGetInstanceUUID(const UUID& uuid)
{
	Expects(uuid.isValid());
	
	if (uuid == instanceUUID) {
		return this;
	}

	for (auto& c: children) {
		auto* result = c.tryGetInstanceUUID(uuid);
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

void EntityData::setIcon(String icon)
{
	this->icon = std::move(icon);
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
	if (delta.icon) {
		icon = delta.icon.value();
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
		std_ex::erase_if(children, [&] (const auto& child) { return child.matchesUUID(childId); });
	}
	for (const auto& child: delta.childrenChanged) {
		auto iter = std_ex::find_if(children, [&] (const auto& cur) { return cur.matchesUUID(child.first); });
		if (iter != children.end()) {
			iter->applyDelta(child.second);
		} else {
			Logger::logWarning("Child not found: " + child.first.toString());
		}
	}
	for (const auto& child: delta.childrenAdded) {
		auto iter = std_ex::find_if(children, [&] (const auto& cur) { return cur.matchesUUID(child); });
		if (iter == children.end()) {
			children.emplace_back(child);
		} else {
			Logger::logWarning("Child already present: " + child.getPrefabUUID().toString());
		}
	}

	if (!delta.childrenOrder.empty()) {
		// TODO
	}
	
	for (const auto& componentId: delta.componentsRemoved) {
		std_ex::erase_if(components, [&] (const auto& component) { return component.first == componentId; });
	}
	for (const auto& component: delta.componentsChanged) {
		auto iter = std_ex::find_if(components, [&] (const auto& cur) { return cur.first == component.first; });
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

bool EntityData::matchesUUID(const UUID& uuid) const
{
	Expects(uuid.isValid());
	return prefabUUID == uuid || instanceUUID == uuid; 
}

bool EntityData::matchesUUID(const EntityData& other) const
{
	return (prefabUUID.isValid() && prefabUUID == other.getPrefabUUID())
		|| (instanceUUID.isValid() && instanceUUID == other.getInstanceUUID());
}

void EntityData::instantiateWith(const EntityData& instance)
{
	// This should only be called on the root of prefab
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
		Logger::logWarning("Untested code at EntityData::updateChild");
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

bool EntityData::isDelta() const
{
	return false;
}

const EntityData& EntityData::asEntityData() const
{
	return *this;
}

const EntityDataDelta& EntityData::asEntityDataDelta() const
{
	throw Exception("Not an EntityDataDelta", HalleyExceptions::Entity);
}

void EntityData::setSceneRoot(bool root)
{
	sceneRoot = root;
}

bool EntityData::isSceneRoot() const
{
	return sceneRoot;
}

std::optional<size_t> EntityData::getChildIndex(const UUID& uuid) const
{
	for (size_t i = 0; i < children.size(); ++i) {
		if (children[i].getInstanceUUID() == uuid) {
			return i;
		}
	}
	return {};
}
