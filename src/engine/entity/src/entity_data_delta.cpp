#include "entity_data_delta.h"

#include <cassert>

#include "entity_data.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/support/logger.h"

using namespace Halley;


EntityDataDelta::Options::Options()
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
	if (from.icon != to.icon) {
		icon = to.icon;
	}
	if (from.flags != to.flags) {
		flags = to.flags;
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
	if (!options.shallow) {
		for (const auto& toChild: to.children) {
			const auto fromIter = std::find_if(from.children.begin(), from.children.end(), [&] (const EntityData& e) { return e.matchesUUID(toChild); });
			if (fromIter != from.children.end()) {
				// Potentially modified
				auto delta = EntityDataDelta(*fromIter, toChild, options);
				if (delta.hasChange()) {
					childrenChanged.emplace_back(toChild.prefabUUID, std::move(delta));
				}
			} else {
				// Inserted
				childrenAdded.emplace_back(toChild);
			}
		}
		for (const auto& fromChild: from.children) {
			const bool stillExists = std::find_if(to.children.begin(), to.children.end(), [&] (const EntityData& e) { return e.matchesUUID(fromChild); }) != to.children.end();
			if (!stillExists) {
				// Removed
				assert(fromChild.getPrefabUUID().isValid());
				childrenRemoved.emplace_back(fromChild.getPrefabUUID());
			}
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
				if (delta.getType() == ConfigNodeType::DeltaMap && !delta.asMap().empty()) {
					componentsChanged.emplace_back(toComponent.first, std::move(delta));
				}
			} else {
				// Inserted
				componentsChanged.emplace_back(toComponent.first, ConfigNode::createDelta(ConfigNode::MapType(), toComponent.second));
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

bool EntityDataDelta::hasChange() const
{
	return name || prefab || icon || flags || instanceUUID || prefabUUID || parentUUID
		|| !componentsChanged.empty() || !componentsRemoved.empty() || !componentOrder.empty()
		|| !childrenChanged.empty() || !childrenAdded.empty() || !childrenRemoved.empty() || !childrenOrder.empty();
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
	encodeField(childrenAdded, FieldId::ChildrenAdded);
	encodeField(childrenRemoved, FieldId::ChildrenRemoved);
	encodeField(childrenOrder, FieldId::ChildrenOrder);
	encodeField(componentsChanged, FieldId::ComponentsChanged);
	encodeField(componentsRemoved, FieldId::ComponentsRemoved);
	encodeField(componentOrder, FieldId::ComponentsOrder);
	encodeOptField(icon, FieldId::Icon);
	encodeOptField(flags, FieldId::Flags);
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
	decodeField(childrenAdded, FieldId::ChildrenAdded);
	decodeField(childrenRemoved, FieldId::ChildrenRemoved);
	decodeField(childrenOrder, FieldId::ChildrenOrder);
	decodeField(componentsChanged, FieldId::ComponentsChanged);
	decodeField(componentsRemoved, FieldId::ComponentsRemoved);
	decodeField(componentOrder, FieldId::ComponentsOrder);
	decodeOptField(icon, FieldId::Icon);
	decodeOptField(flags, FieldId::Flags);
}

void EntityDataDelta::setPrefabUUID(const UUID& uuid)
{
	prefabUUID = uuid;
}

bool EntityDataDelta::isSimpleDelta() const
{
	if (!childrenAdded.empty() || !childrenRemoved.empty() || prefab) {
		return false;
	}

	for (auto& child: childrenChanged) {
		if (!child.second.isSimpleDelta()) {
			return false;
		}
	}

	return true;
}

bool EntityDataDelta::isDelta() const
{
	return true;
}

const EntityData& EntityDataDelta::asEntityData() const
{
	throw Exception("Not an EntityData", HalleyExceptions::Entity);
}

const EntityDataDelta& EntityDataDelta::asEntityDataDelta() const
{
	return *this;
}

bool EntityDataDelta::modifiesTheSameAs(const EntityDataDelta& other) const
{
	if (getFieldsPresent() != other.getFieldsPresent()) {
		return false;
	}

	if (!childrenAdded.empty() || !childrenRemoved.empty() || !childrenChanged.empty() || !childrenOrder.empty()) {
		return false;
	}

	if (!componentsRemoved.empty() || !componentOrder.empty()) {
		return false;
	}

	if (componentsChanged.size() != other.componentsChanged.size()) {
		return false;
	}

	return getComponentEmptyStructure() == other.getComponentEmptyStructure();
}

static ConfigNode getEmptyConfigNodeStructure(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map || node.getType() == ConfigNodeType::DeltaMap) {
		ConfigNode::MapType result;
		for (const auto& [k, v]: node.asMap()) {
			result[k] = getEmptyConfigNodeStructure(v);
		}
		return result;
	}
	if (node.getType() == ConfigNodeType::Sequence || node.getType() == ConfigNodeType::DeltaSequence) {
		ConfigNode::SequenceType result;
		for (const auto& v: node.asSequence()) {
			result.emplace_back(getEmptyConfigNodeStructure(v));
		}
		return result;
	}
	return ConfigNode();
}

std::vector<std::pair<String, ConfigNode>> EntityDataDelta::getComponentEmptyStructure() const
{
	std::vector<std::pair<String, ConfigNode>> result;

	for (const auto& c: componentsChanged) {
		result.emplace_back(c.first, getEmptyConfigNodeStructure(c.second));
	}
	
	return result;
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
	checkFieldVec(childrenAdded, FieldId::ChildrenAdded);
	checkFieldVec(childrenRemoved, FieldId::ChildrenRemoved);
	checkFieldVec(childrenOrder, FieldId::ChildrenOrder);
	checkFieldVec(componentsChanged, FieldId::ComponentsChanged);
	checkFieldVec(componentsRemoved, FieldId::ComponentsRemoved);
	checkFieldVec(componentOrder, FieldId::ComponentsOrder);
	checkField(icon, FieldId::Icon);
	checkField(flags, FieldId::Flags);

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
