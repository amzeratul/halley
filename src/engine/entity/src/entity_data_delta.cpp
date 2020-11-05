#include "entity_data_delta.h"
#include "entity_data.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/support/logger.h"

using namespace Halley;


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
	return name || prefab || instanceUUID || prefabUUID || parentUUID
		|| !componentsChanged.empty() || !componentsRemoved.empty() || !componentOrder.empty()
		|| !childrenChanged.empty() || !childrenRemoved.empty() || !childrenOrder.empty();
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

void EntityDataDelta::setPrefabUUID(const UUID& uuid)
{
	prefabUUID = uuid;
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
