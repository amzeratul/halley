#include "halley/entity/entity_data.h"
#include "halley/entity/entity_data_delta.h"

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

	if (data) {
		if (data->getType() == IEntityData::Type::Delta) {
			result.data = std::make_unique<EntityDataDelta>(dynamic_cast<EntityDataDelta&>(*data));
		} else {
			result.data = std::make_unique<EntityData>(dynamic_cast<EntityData&>(*data));
		}
	}

	return result;
}

bool EntityChangeOperation::operator==(const EntityChangeOperation& other) const
{
	if (!!data ^ !other.data) {
		return false;
	}
	if (data && other.data) {
		if (data->getType() != other.data->getType()) {
			return false;
		}
		// TODO, compare datas
	}

	return entityId == other.entityId && parent == other.parent && childIndex == other.childIndex;
}

bool EntityChangeOperation::operator!=(const EntityChangeOperation& other) const
{
	return !(*this == other);
}

bool IEntityConcreteData::hasChildWithUUID(const UUID& uuid) const
{
	const auto n = getNumChildren();
	for (size_t i = 0; i < n; ++i) {
		if (getChild(i).getInstanceUUID() == uuid) {
			return true;
		}
	}
	return false;
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
	flags = static_cast<uint8_t>(data["flags"].asInt(0));

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
	if (flags != 0) {
		result["flags"] = static_cast<int>(flags);
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
	options.mapKeyOrder = {{ "name", "prefab", "icon", "flags", "uuid", "prefabUUID", "parent", "components", "children" }};
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

bool EntityData::getFlag(Flag flag) const
{
	return (flags & static_cast<uint8_t>(flag)) != 0;
}

size_t EntityData::getNumChildren() const
{
	return children.size();
}

const IEntityConcreteData& EntityData::getChild(size_t idx) const
{
	return children[idx];
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

size_t EntityData::getNumComponents() const
{
	return components.size();
}

const std::pair<String, ConfigNode>& EntityData::getComponent(size_t idx) const
{
	return components[idx];
}

bool EntityData::fillEntityDataStack(Vector<const EntityData*>& stack, const UUID& entityId) const
{
	if (getInstanceUUID() == entityId) {
		return true;
	}

	for (auto& c : getChildren()) {
		if (c.fillEntityDataStack(stack, entityId)) {
			stack.push_back(this);
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

const ConfigNode& EntityData::getFieldData(const String& componentName, const String& fieldName) const
{
	static ConfigNode undefined;
	
	for (const auto& c: components) {
		if (c.first == componentName) {
			if (c.second.hasKey(fieldName)) {
				return c.second[fieldName];
			}
		}
	}

	return undefined;
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

bool EntityData::setFlag(Flag f, bool value)
{
	const auto flag = static_cast<uint8_t>(f);
	const auto oldValue = flags;
	flags = (flags & ~flag) | (value ? flag : 0);
	return flags != oldValue;
}

void EntityData::randomiseInstanceUUIDs()
{
	instanceUUID = UUID::generate();
	for (auto& c: children) {
		c.randomiseInstanceUUIDs();
	}
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

void EntityData::setChildren(Vector<EntityData> children)
{
	this->children = std::move(children);
}

void EntityData::setComponents(Vector<std::pair<String, ConfigNode>> components)
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
	if (delta.flags) {
		flags = delta.flags.value();
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
			children.push_back(EntityData(child.second));
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

void EntityData::instantiate(const UUID& uuid)
{
	instanceUUID = uuid;
	generateChildUUID(uuid);
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

	flags |= instance.flags;
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

IEntityData::Type EntityData::getType() const
{
	return Type::Data;
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

size_t EntityData::getSizeBytes() const
{
	size_t result = sizeof(EntityData);
	result += name.getSizeBytes();
	result += prefab.getSizeBytes();
	result += icon.getSizeBytes();

	for (const auto& c: children) {
		result += c.getSizeBytes();
	}
	for (const auto& c: components) {
		result += c.first.getSizeBytes();
		result += c.second.getSizeBytes();
	}

	return result;
}

void EntityData::generateUUIDs(HashMap<UUID, UUID>& changes)
{
	const auto newValue = UUID::generate();
	changes[instanceUUID] = newValue;
	instanceUUID = newValue;

	if (parentUUID.isValid()) {
		parentUUID = changes.at(parentUUID);
	}

	for (auto& c: children) {
		c.generateUUIDs(changes);
	}
}

namespace {
	void replaceUUIDInConfigNode(ConfigNode& value, const HashMap<UUID, UUID>& changes)
	{
		if (value.getType() == ConfigNodeType::String) {
			const auto uuid = UUID::tryParse(value.asString());
			if (uuid) {
				const auto iter = changes.find(*uuid);
				if (iter != changes.end()) {
					value = iter->second.toString();
				}
			}
		} else if (value.getType() == ConfigNodeType::Sequence) {
			for (auto& v: value.asSequence()) {
				replaceUUIDInConfigNode(v, changes);
			}
		} else if (value.getType() == ConfigNodeType::Map) {
			for (auto& [k, v]: value.asMap()) {
				replaceUUIDInConfigNode(v, changes);
			}
		}
	}
}

void EntityData::updateComponentUUIDs(const HashMap<UUID, UUID>& changes)
{
	for (auto& [k, v]: components) {
		replaceUUIDInConfigNode(v, changes);
	}

	for (auto& c: children) {
		c.updateComponentUUIDs(changes);
	}
}
