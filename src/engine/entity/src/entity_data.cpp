#include "entity_data.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/support/logger.h"
using namespace Halley;

EntityData::EntityData()
{}

EntityData::EntityData(UUID instanceUUID)
	: instanceUUID(instanceUUID)
{
}

EntityData::EntityData(const ConfigNode& data)
{
	name = data["name"].asString("");
	prefab = data["prefab"].asString("");
	parseUUID(instanceUUID, data["uuid"]);
	parseUUID(prefabUUID, data["prefabUUID"]);
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
	if (uuid == prefabUUID) {
		return this;
	}

	for (const auto& c: children) {
		const auto result = c.tryGetPrefabUUID(uuid);
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

EntityDataDelta::Options::Options()
	: preserveOrder(false)
{
}

EntityDataDelta::EntityDataDelta()
{}

EntityDataDelta::EntityDataDelta(const EntityData& to, const Options& options)
	: EntityDataDelta(EntityData(), to, options)
{
}

EntityDataDelta::EntityDataDelta(const EntityData& from, const EntityData& to, const Options& options)
{
	if (from.name != to.name) {
		name = to.name;
	}
	if (from.prefab != to.prefab) {
		prefab = to.prefab;
	}
	if (from.instanceUUID != to.instanceUUID) {
		instanceUUID = to.instanceUUID;
	}
	if (from.prefabUUID != to.prefabUUID) {
		prefabUUID = to.prefabUUID;
	}
	if (from.parentUUID != to.parentUUID) {
		parentUUID = to.parentUUID;
	}

	// Children
	for (const auto& toChild: to.children) {
		const auto fromIter = std::find_if(from.children.begin(), from.children.end(), [&] (const EntityData& e) { return e.isSameEntity(toChild); });
		if (fromIter != from.children.end()) {
			// Potentially modified
			auto delta = EntityDataDelta(*fromIter, toChild, options);
			if (delta.hasChange()) {
				childrenChanged.emplace_back(toChild.prefabUUID, std::move(delta));
			}
		} else {
			// Inserted
			childrenChanged.emplace_back(toChild.prefabUUID, EntityDataDelta(EntityData(), toChild, options));
		}
	}
	for (const auto& fromChild: from.children) {
		const bool stillExists = std::find_if(to.children.begin(), to.children.end(), [&] (const EntityData& e) { return e.isSameEntity(fromChild); }) != to.children.end();
		if (!stillExists) {
			// Removed
			childrenRemoved.emplace_back(fromChild.getPrefabUUID());
		}
	}
	if (options.preserveOrder) {
		// TODO
	}

	// Components
	for (const auto& toComponent: to.components) {
		const String& compId = toComponent.first;
		if (options.ignoreComponents.find(compId) == options.ignoreComponents.end()) {
			const auto fromIter = std::find_if(from.components.begin(), from.components.end(), [&] (const auto& e) { return e.first == toComponent.first; });
			if (fromIter != from.components.end()) {
				// Potentially modified
				auto delta = ConfigNode::createDelta(fromIter->second, toComponent.second);
				if (delta.getType() != ConfigNodeType::Noop) {
					componentsChanged.emplace_back(toComponent.first, std::move(delta));
				}
			} else {
				// Inserted
				componentsChanged.emplace_back(toComponent);
			}
		}
	}
	for (const auto& fromComponent: from.components) {
		const String& compId = fromComponent.first;
		if (options.ignoreComponents.find(compId) == options.ignoreComponents.end()) {
			const bool stillExists = std::find_if(to.components.begin(), to.components.end(), [&] (const auto& e) { return e.first == compId; }) != to.components.end();
			if (!stillExists) {
				// Removed
				componentsRemoved.emplace_back(compId);
			}
		}
	}
	if (options.preserveOrder) {
		// TODO
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

bool EntityDataDelta::hasChange() const
{
	return name || prefab || instanceUUID || prefabUUID || parentUUID
		|| !componentsChanged.empty() || !componentsRemoved.empty() || !componentOrder.empty()
		|| !childrenChanged.empty() || !childrenRemoved.empty() || !childrenOrder.empty();
}

bool EntityData::isSameEntity(const EntityData& other) const
{
	return prefabUUID == other.prefabUUID;
}

void EntityData::instantiateWith(const EntityData& instance)
{
	// Root of prefab
	prefabUUID = instanceUUID;
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
	prefabUUID = instanceUUID;
	instanceUUID = UUID::generateFromUUIDs(instanceUUID, root);

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
		if (c.instanceUUID == instanceChildData.instanceUUID) { // Theoretically prefabUUIDs should match here too
			c.instantiateData(c);
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

void EntityDataDelta::serialize(Serializer& s) const
{
	uint16_t fieldsPresent = getFieldsPresent();
	s << fieldsPresent;

	auto encodeField = [&] (auto& v, FieldId id)
	{
		if (isFieldPresent(fieldsPresent, id)) {
			s << v;
		}
	};

	auto encodeOptField = [&] (auto& v, FieldId id)
	{
		if (isFieldPresent(fieldsPresent, id)) {
			s << v.value();
		}
	};

	encodeOptField(name, FieldId::Name);
	encodeOptField(prefab, FieldId::Prefab);
	encodeOptField(instanceUUID, FieldId::InstanceUUID);
	encodeOptField(prefabUUID, FieldId::PrefabUUID);
	encodeOptField(parentUUID, FieldId::ParentUUID);
	encodeField(childrenChanged, FieldId::ChildrenChanged);
	encodeField(childrenRemoved, FieldId::ChildrenRemoved);
	encodeField(childrenOrder, FieldId::ChildrenOrder);
	encodeField(componentsChanged, FieldId::ComponentsChanged);
	encodeField(componentsRemoved, FieldId::ComponentsRemoved);
	encodeField(componentOrder, FieldId::ComponentsOrder);
}

void EntityDataDelta::deserialize(Deserializer& s)
{
	uint16_t fieldsPresent;
	s >> fieldsPresent;

	auto decodeField = [&] (auto& v, FieldId id)
	{
		if (isFieldPresent(fieldsPresent, id)) {
			s >> v;
		}
	};

	auto decodeOptField = [&] (auto& v, FieldId id)
	{
		if (isFieldPresent(fieldsPresent, id)) {
			std::remove_reference_t<decltype(*v)> tmp;
			s >> tmp;
			v = std::move(tmp);
		}
	};
	
	decodeOptField(name, FieldId::Name);
	decodeOptField(prefab, FieldId::Prefab);
	decodeOptField(instanceUUID, FieldId::InstanceUUID);
	decodeOptField(prefabUUID, FieldId::PrefabUUID);
	decodeOptField(parentUUID, FieldId::ParentUUID);
	decodeField(childrenChanged, FieldId::ChildrenChanged);
	decodeField(childrenRemoved, FieldId::ChildrenRemoved);
	decodeField(childrenOrder, FieldId::ChildrenOrder);
	decodeField(componentsChanged, FieldId::ComponentsChanged);
	decodeField(componentsRemoved, FieldId::ComponentsRemoved);
	decodeField(componentOrder, FieldId::ComponentsOrder);
}

uint16_t EntityDataDelta::getFieldBit(FieldId id)
{
	return static_cast<uint16_t>(1 << static_cast<int>(id));
}

void EntityDataDelta::setFieldPresent(uint16_t& value, FieldId id, bool present)
{
	if (present) {
		value |= getFieldBit(id);
	} else {
		value &= ~getFieldBit(id);
	}
}

bool EntityDataDelta::isFieldPresent(uint16_t value, FieldId id)
{
	return (value & getFieldBit(id)) != 0;
}

uint16_t EntityDataDelta::getFieldsPresent() const
{
	uint16_t value = 0;
	
	auto checkField = [&] (const auto& v, FieldId id)
	{
		if (v) {
			setFieldPresent(value, id, true);
		}
	};

	auto checkFieldVec = [&] (const auto& v, FieldId id)
	{
		if (!v.empty()) {
			setFieldPresent(value, id, true);
		}
	};

	checkField(name, FieldId::Name);
	checkField(prefab, FieldId::Prefab);
	checkField(instanceUUID, FieldId::InstanceUUID);
	checkField(prefabUUID, FieldId::PrefabUUID);
	checkField(parentUUID, FieldId::ParentUUID);
	checkFieldVec(childrenChanged, FieldId::ChildrenChanged);
	checkFieldVec(childrenRemoved, FieldId::ChildrenRemoved);
	checkFieldVec(childrenOrder, FieldId::ChildrenOrder);
	checkFieldVec(componentsChanged, FieldId::ComponentsChanged);
	checkFieldVec(componentsRemoved, FieldId::ComponentsRemoved);
	checkFieldVec(componentOrder, FieldId::ComponentsOrder);

	return value;
}

void SceneDataDelta::addEntity(UUID entityId, EntityDataDelta delta)
{
	entities.emplace_back(std::move(entityId), std::move(delta));
}

const std::vector<std::pair<UUID, EntityDataDelta>>& SceneDataDelta::getEntities() const
{
	return entities;
}

void SceneDataDelta::serialize(Serializer& s) const
{
	s << entities;
}

void SceneDataDelta::deserialize(Deserializer& s)
{
	s >> entities;
}
