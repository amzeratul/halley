#include "halley/entity/entity_data_delta.h"

#include <cassert>

#include "halley/entity/data_interpolator.h"
#include "halley/entity/entity_data.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/file_formats/yaml_convert.h"
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
	if (from.variant != to.variant) {
		variant = to.variant;
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
			if (!options.allowNonSerializable && toChild.getFlag(IEntityConcreteData::Flag::NotSerializable)) {
				// Ignore non-serializables
				continue;
			}
			const auto fromIter = std::find_if(from.children.begin(), from.children.end(), [&] (const EntityData& e)
			{
				const auto combined = UUID::generateFromUUIDs(e.getPrefabUUID(), to.getInstanceUUID());
			    return e.matchesUUID(toChild) || toChild.matchesUUID(combined);
			});
			if (fromIter != from.children.end()) {
				// Potentially modified
				auto delta = EntityDataDelta(*fromIter, toChild, options);
				if (delta.hasChange()) {
					childrenChanged.emplace_back(toChild.instanceUUID.isValid() ? toChild.instanceUUID : toChild.prefabUUID, std::move(delta));
				}
			} else {
				// Inserted
				childrenAdded.emplace_back(toChild);
			}
		}
		for (const auto& fromChild: from.children) {
			if (!options.allowNonSerializable && fromChild.getFlag(IEntityConcreteData::Flag::NotSerializable)) {
				// Ignore non-serializables
				continue;
			}
			const bool stillExists = std::find_if(to.children.begin(), to.children.end(), [&] (const EntityData& e)
			{
				const auto combined = UUID::generateFromUUIDs(fromChild.getPrefabUUID(), to.getInstanceUUID());
			    return e.matchesUUID(fromChild) || e.matchesUUID(combined);
			}) != to.children.end();
			if (!stillExists) {
				// Removed
				childrenRemoved.emplace_back(fromChild.instanceUUID.isValid() ? fromChild.instanceUUID : fromChild.prefabUUID);
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
				// Potentially modified, compute delta
				ConfigNode delta;
				if (options.interpolatorSet) {
					delta = options.interpolatorSet->createComponentDelta(from.getInstanceUUID(), fromIter->first, fromIter->second, toComponent.second);
				} else {
					delta = ConfigNode::createDelta(fromIter->second, toComponent.second);
				}
				
				if (delta.getType() == ConfigNodeType::DeltaMap && !delta.asMap().empty()) {
					if (options.deltaComponents) {
						componentsChanged.emplace_back(toComponent.first, std::move(delta));
					} else {
						componentsChanged.emplace_back(toComponent.first, toComponent.second);
					}
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
	return name || prefab || icon || variant || flags || instanceUUID || prefabUUID || parentUUID
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
	encodeOptField(variant, FieldId::Variant);
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
	decodeOptField(variant, FieldId::Variant);
}

void EntityDataDelta::setInstanceUUID(const UUID& uuid)
{
	instanceUUID = uuid;
}

void EntityDataDelta::setPrefabUUID(const UUID& uuid)
{
	prefabUUID = uuid;
}

void EntityDataDelta::randomiseInstanceUUIDs()
{
	instanceUUID = UUID::generate();
	for (auto& c: childrenAdded) {
		c.randomiseInstanceUUIDs();
	}
	for (auto& c: childrenChanged) {
		c.second.randomiseInstanceUUIDs();
	}
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

IEntityData::Type EntityDataDelta::getType() const
{
	return Type::Delta;
}

void EntityDataDelta::instantiate(const UUID& uuid)
{
	for (auto& c: childrenAdded) {
		c.instantiate(uuid);
	}
	for (auto& c: childrenChanged) {
		c.first = UUID::generateFromUUIDs(uuid, c.first);
		c.second.instantiate(uuid);
	}
	for (auto& c: childrenRemoved) {
		c = UUID::generateFromUUIDs(uuid, c);
	}
	for (auto& c: childrenOrder) {
		c = UUID::generateFromUUIDs(uuid, c);
	}
}

EntityDataDelta EntityDataDelta::instantiateAsCopy(const UUID& uuid) const
{
	EntityDataDelta result = *this;
	result.instantiate(uuid);
	return result;
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

ConfigNode EntityDataDelta::toConfigNode() const
{
	ConfigNode::MapType result;

	if (name) {
		result["name"] = name.value();
	}
	if (prefab) {
		result["prefab"] = prefab.value();
	}
	if (icon) {
		result["icon"] = icon.value();
	}
	if (variant) {
		result["variant"] = variant.value();
	}
	if (instanceUUID) {
		result["uuid"] = instanceUUID->toString();
	}
	if (prefabUUID) {
		result["prefabUUID"] = prefabUUID->toString();
	}
	if (parentUUID) {
		result["parent"] = parentUUID->toString();
	}
	if (flags) {
		result["flags"] = static_cast<int>(flags.value());
	}

	if (!componentsChanged.empty()) {
		ConfigNode::SequenceType compNodes;
		for (const auto& comp: componentsChanged) {
			ConfigNode::MapType entry;
			entry[comp.first] = ConfigNode(comp.second);
			compNodes.emplace_back(std::move(entry));
		}
		result["componentsChanged"] = std::move(compNodes);
	}

	if (!componentsRemoved.empty()) {
		result["componentsRemoved"] = componentsRemoved;
	}

	if (!componentOrder.empty()) {
		result["componentOrder"] = componentOrder;
	}

	if (!childrenAdded.empty()) {
		ConfigNode::SequenceType childNodes;
		for (const auto& child: childrenAdded) {
			childNodes.emplace_back(child.toConfigNode(true));
		}
		result["childrenAdded"] = std::move(childNodes);
	}

	if (!childrenChanged.empty()) {
		ConfigNode::SequenceType childNodes;
		for (const auto& child: childrenChanged) {
			ConfigNode::MapType entry;
			entry[toString(child.first)] = ConfigNode(child.second.toConfigNode());
			childNodes.emplace_back(std::move(entry));
		}
		result["childrenChanged"] = std::move(childNodes);
	}

	if (!childrenRemoved.empty()) {
		result["childrenRemoved"] = childrenRemoved;
	}

	if (!childrenOrder.empty()) {
		result["childrenOrder"] = childrenOrder;
	}
	
	return ConfigNode(std::move(result));
}

String EntityDataDelta::toYAML() const
{
	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = {{ "name", "prefab", "icon", "flags", "uuid", "prefabUUID", "parent", "components", "children" }};
	return YAMLConvert::generateYAML(toConfigNode(), options);
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

Vector<std::pair<String, ConfigNode>> EntityDataDelta::getComponentEmptyStructure() const
{
	Vector<std::pair<String, ConfigNode>> result;

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
	checkField(variant, FieldId::Variant);

	return value;
}

void SceneDataDelta::addEntity(UUID entityId, EntityDataDelta delta)
{
	entities.emplace_back(std::move(entityId), std::move(delta));
}

const Vector<std::pair<UUID, EntityDataDelta>>& SceneDataDelta::getEntities() const
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
