#include "halley/entity/entity_data_delta.h"

#include "halley/net/interpolators/data_interpolator.h"
#include "halley/entity/entity_data.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/entity/world_reflection.h"
#include "halley/file_formats/yaml_convert.h"

using namespace Halley;


EntityDataDelta::Options::Options() = default;

EntityDataDelta::EntityDataDelta() = default;

EntityDataDelta::EntityDataDelta(const EntityData& to, const Options& options)
	: EntityDataDelta(EntityData(), to, options)
{
}

EntityDataDelta::EntityDataDelta(const EntityData& from, const EntityData& to, const Options& options)
	: EntityDataDelta(from, to, to.getInstanceUUID(), options)
{
}

EntityDataDelta::EntityDataDelta(const EntityData& from, const EntityData& to, const UUID& rootUUID, const Options& options)
{
	if (from.prefab != to.prefab) {
		prefab = to.prefab;
	}
	if (from.name != to.name) {
		if (prefab || to.prefab.isEmpty() || !options.ignoreNameAndIconChangesInInstances) {
			name = to.name;
		}
	}
	if (from.icon != to.icon) {
		if (prefab || to.prefab.isEmpty() || !options.ignoreNameAndIconChangesInInstances) {
			icon = to.icon;
		}
	}
	if (from.variant != to.variant) {
		variant = to.variant;
	}
	if (from.enableRules != to.enableRules) {
		enableRules = to.enableRules;
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
			// Try to find a matching child in source data.
			bool alreadyExists = false;
			for (const auto& fromChild : from.children) {
				if (fromChild.prefabUUID == toChild.prefabUUID) {
					// This might be an instantiated prefab. Mirror the UUID generation in
					// EntityData::doInstantiatePrefabs().
					const auto combined = rootUUID ^ fromChild.prefabUUID;
					alreadyExists = fromChild.matchesUUID(combined);
				}

				if (!alreadyExists) {
					const auto combined = fromChild.prefabUUID ^ to.instanceUUID;
					alreadyExists = fromChild.matchesUUID(toChild) || toChild.matchesUUID(combined);
				}

				if (alreadyExists) {
					// Potentially modified
					auto delta = EntityDataDelta(fromChild, toChild, rootUUID, options);
					if (delta.hasChange()) {
						childrenChanged.emplace_back(toChild.instanceUUID.isValid() ? toChild.instanceUUID : toChild.prefabUUID, std::move(delta));
					}
					break;
				}
			}
			if (!alreadyExists) {
				// Inserted
				childrenAdded.emplace_back(toChild);
				childrenAdded.back().postProcessAddedChild(options.ignoreComponents, options.omitEmptyComponents);
			}
		}
		for (const auto& fromChild: from.children) {
			if (!options.allowNonSerializable && fromChild.getFlag(IEntityConcreteData::Flag::NotSerializable)) {
				// Ignore non-serializables
				continue;
			}
			bool stillExists = false;
			for (const auto& toChild : to.children) {
				if (fromChild.prefabUUID == toChild.prefabUUID) {
					// This might be an instantiated prefab. Mirror the UUID generation in
					// EntityData::doInstantiatePrefabs().
					const auto combined = rootUUID ^ fromChild.prefabUUID;
					stillExists = fromChild.matchesUUID(combined);
				}

				if (!stillExists) {
					const auto combined = fromChild.prefabUUID ^ to.instanceUUID;
					stillExists = toChild.matchesUUID(fromChild) || toChild.matchesUUID(combined);
				}

				if (stillExists) {
					break;
				}
			}
			if (!stillExists) {
				// Removed
				childrenRemoved.emplace_back(fromChild.instanceUUID.isValid() ? fromChild.instanceUUID : fromChild.prefabUUID);
			}
		}
	}
	if (options.preserveChildOrder) {
		// TODO
	}

	// Components
	for (const auto& toComponent: to.components) {
		const String& compId = toComponent.first;

		if (!options.ignoreComponents.contains(compId)) {
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
				if (!options.ignoreInsertComponents.contains(compId)) {
					componentsChanged.emplace_back(toComponent.first, ConfigNode::createDelta(ConfigNode::MapType(), toComponent.second));
				}
			}
		}
	}
	if (!options.ignoreComponentsRemoved) {
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
	}
	if (options.preserveComponentOrder) {
		Vector<String> fromOrder;
		Vector<String> toOrder;
		for (const auto& c: from.components) {
			fromOrder.push_back(c.first);
		}
		for (const auto& c: to.components) {
			toOrder.push_back(c.first);
		}
		if (fromOrder != toOrder) {
			componentOrder = std::move(toOrder);
		}
	}
}

bool EntityDataDelta::hasChange() const
{
	// Checking instance/prefab UUID causes issues with spurious serialisation of entities - if they cause other issues, this method might need to be split/take parameters
	return name || prefab || icon || variant || enableRules || flags || parentUUID /*|| instanceUUID || prefabUUID*/
		|| !componentsChanged.empty() || !componentsRemoved.empty() || !componentOrder.empty()
		|| !childrenChanged.empty() || !childrenAdded.empty() || !childrenRemoved.empty() || !childrenOrder.empty();
}

void EntityDataDelta::serialize(Serializer& s) const
{
	uint32_t fieldsPresent = getFieldsPresent();
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
	encodeOptField(enableRules, FieldId::EnableRules);
}

void EntityDataDelta::deserialize(Deserializer& s)
{
	uint32_t fieldsPresent;
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
	decodeOptField(enableRules, FieldId::EnableRules);

	if (deserializeChildrenComponentsAsDeltas) {
		for (auto& c : childrenAdded) {
			c.makeComponentChangesIntoDeltas();
		}
	}

	assignChildUUIDs();
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

void EntityDataDelta::assignChildUUIDs()
{
	for (auto& c: childrenChanged) {
		c.second.setInstanceUUID(c.first);
		c.second.assignChildUUIDs();
	}
}

ConfigNode* EntityDataDelta::tryGetComponentChanged(const String& compName)
{
	for (auto& [k, v]: componentsChanged) {
		if (k == compName) {
			return &v;
		}
	}
	return nullptr;
}

const ConfigNode* EntityDataDelta::tryGetComponentChanged(const String& compName) const
{
	for (auto& [k, v]: componentsChanged) {
		if (k == compName) {
			return &v;
		}
	}
	return nullptr;
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
		c.first = UUID::xorUUIDs(uuid, c.first);
		c.second.instantiate(uuid);
	}
	for (auto& c: childrenRemoved) {
		c = UUID::xorUUIDs(uuid, c);
	}
	for (auto& c: childrenOrder) {
		c = UUID::xorUUIDs(uuid, c);
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
	if (enableRules) {
		result["enableRules"] = enableRules.value();
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

bool EntityDataDelta::operator==(const EntityDataDelta& other) const
{
	return name == other.name &&
		prefab == other.prefab &&
		icon == other.icon &&
		variant == other.variant &&
		enableRules == other.enableRules &&
		flags == other.flags &&
		instanceUUID == other.instanceUUID &&
		prefabUUID == other.prefabUUID &&
		parentUUID == other.parentUUID &&
		componentsChanged == other.componentsChanged &&
		componentsRemoved == other.componentsRemoved &&
		componentOrder == other.componentOrder &&
		childrenAdded == other.childrenAdded &&
		childrenChanged == other.childrenChanged &&
		childrenRemoved == other.childrenRemoved &&
		childrenOrder == other.childrenOrder;
}

bool EntityDataDelta::operator!=(const EntityDataDelta& other) const
{
	return !(*this == other);
}

void EntityDataDelta::sanitize(const WorldReflection& worldReflection, int mask)
{
	for (auto& child: childrenAdded) {
		child.sanitize(worldReflection, mask);
	}

	for (auto& child: childrenChanged) {
		child.second.sanitize(worldReflection, mask);
	}

	for (auto& [componentId, data]: componentsChanged) {
		worldReflection.getComponentReflector(componentId).sanitize(data, mask);
	}
}

void EntityDataDelta::stripComponentChanges(std::string_view componentName)
{
	std_ex::erase_if_key(componentsChanged, [&] (const auto& k) { return k == componentName; });
	std_ex::erase(componentsRemoved, componentName);

	for (auto& child: childrenChanged) {
		child.second.stripComponentChanges(componentName);
	}
}

void EntityDataDelta::stripAllComponentRemove()
{
	componentsRemoved.clear();

	for (auto& child: childrenChanged) {
		child.second.stripAllComponentRemove();
	}
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

uint32_t EntityDataDelta::getFieldBit(FieldId id)
{
	return static_cast<uint32_t>(1 << static_cast<int>(id));
}

void EntityDataDelta::setFieldPresent(uint32_t& value, FieldId id, bool present)
{
	if (present) {
		value |= getFieldBit(id);
	} else {
		value &= ~getFieldBit(id);
	}
}

bool EntityDataDelta::isFieldPresent(uint32_t value, FieldId id)
{
	return (value & getFieldBit(id)) != 0;
}

uint32_t EntityDataDelta::getFieldsPresent() const
{
	uint32_t value = 0;
	
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
	checkField(enableRules, FieldId::EnableRules);

	return value;
}

void SceneDataDelta::Entry::serialize(Serializer& s) const
{
	int version = 1;
	s << version;
	s << uuid;
	s << hash;
	s << entityData;
}

void SceneDataDelta::Entry::deserialize(Deserializer& s)
{
	int version;
	s >> version;
	s >> uuid;
	s >> hash;
	s >> entityData;
}

ConfigNode SceneDataDelta::Entry::toConfigNode() const
{
	ConfigNode result;
	result["uuid"] = uuid;
	result["hash"] = static_cast<int64_t>(hash);
	result["entityData"] = entityData;
	return result;
}

void SceneDataDelta::addEntity(Entry entry)
{
	index[entry.uuid] = static_cast<uint32_t>(entities.size());
	entities.emplace_back(std::move(entry));
}

const Vector<SceneDataDelta::Entry>& SceneDataDelta::getEntities() const
{
	return entities;
}

const SceneDataDelta::Entry* SceneDataDelta::tryGetEntity(const UUID& uuid) const
{
	const auto iter = index.find(uuid);
	if (iter != index.end()) {
		return &entities[iter->second];
	} else {
		return nullptr;
	}
}

void SceneDataDelta::serialize(Serializer& s) const
{
	int version = 2;
	s << version;
	s << entities;
}

void SceneDataDelta::deserialize(Deserializer& s)
{
	int version;
	s >> version;

	if (version == 1) {
		Vector<std::pair<UUID, EntityDataDelta>> es;
		s >> es;
		entities.resize(es.size());
		for (size_t i = 0; i < es.size(); ++i) {
			entities[i].uuid = es[i].first;
			entities[i].entityData = std::move(es[i].second);
		}
	} else if (version == 2) {
		s >> entities;
	} else {
		throw Exception("Unknown SceneDataDelta version: " + toString(version), 0);
	}

	buildIndex();
}

ConfigNode SceneDataDelta::toConfigNode() const
{
	ConfigNode::MapType result;
	result["entities"] = entities;
	return result;
}

void SceneDataDelta::buildIndex()
{
	index.clear();
	for (size_t i = 0; i < entities.size(); ++i) {
		index[entities[i].uuid] = static_cast<uint32_t>(i);
	}
}

ConfigNode ConfigNodeSerializer<EntityDataDelta>::serialize(const EntityDataDelta& entityData, const EntitySerializationContext& context)
{
	return entityData.toConfigNode();
}

EntityDataDelta ConfigNodeSerializer<EntityDataDelta>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	throw Exception("EntityDataDelta deserialization is not implemented", HalleyExceptions::Entity);
}

void ConfigNodeSerializer<EntityDataDelta>::deserialize(const EntitySerializationContext& context, const ConfigNode& node, EntityDataDelta& target)
{
	throw Exception("EntityDataDelta deserialization is not implemented", HalleyExceptions::Entity);
}
