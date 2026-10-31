#include "halley/entity/entity_data_applied_delta.h"

using namespace Halley;

EntityDataAppliedDelta::EntityDataAppliedDelta(const EntityData& base, const EntityDataDelta* delta, const UUID& rootUUID, bool isRoot)
	: base(&base)
	, delta(delta)
	, rootUUID(rootUUID)
	, isRoot(isRoot)
{
	if (isRoot) {
		instanceUUID = rootUUID;
	} else if (delta && delta->getInstanceUUID()) {
		instanceUUID = *delta->getInstanceUUID();
	} else {
		instanceUUID = UUID::xorUUIDs(rootUUID, base.getPrefabUUID());
	}
}

IEntityData::Type EntityDataAppliedDelta::getType() const
{
	return Type::Data;
}

const String& EntityDataAppliedDelta::getName() const
{
	if (!base) [[unlikely]] {
		return String::emptyString();
	}
	
	if (delta) {
		if (const auto& name = delta->getName()) {
			return *name;
		}
	}
	return base->getName();
}

const String& EntityDataAppliedDelta::getPrefab() const
{
	if (isRoot) {
		// It has been applied, return empty on purpose
		return String::emptyString();
	}

	if (!base) [[unlikely]] {
		return String::emptyString();
	}
	
	if (delta) {
		if (const auto& prefab = delta->getPrefab()) {
			return *prefab;
		}
	}
	return base->getPrefab();
}

const String& EntityDataAppliedDelta::getVariant() const
{
	if (!base) [[unlikely]] {
		return String::emptyString();
	}
	
	if (delta) {
		if (const auto& variant = delta->getVariant()) {
			return *variant;
		}
	}
	return base->getVariant();
}

const String& EntityDataAppliedDelta::getEnableRules() const
{
	if (!base) [[unlikely]] {
		return String::emptyString();
	}
	
	if (delta) {
		if (const auto& enableRules = delta->getEnableRules()) {
			return *enableRules;
		}
	}
	return base->getEnableRules();
}

uint8_t EntityDataAppliedDelta::getFlags() const
{
	if (!base) [[unlikely]] {
		return 0;
	}
	
	if (delta) {
		if (const auto& flags = delta->getFlags()) {
			return *flags;
		}
	}
	return base->getFlags();
}

bool EntityDataAppliedDelta::getFlag(Flag flag) const
{
	return (getFlags() & static_cast<uint8_t>(flag)) != 0;
}

const UUID& EntityDataAppliedDelta::getInstanceUUID() const
{
	return instanceUUID;
}

const UUID& EntityDataAppliedDelta::getPrefabUUID() const
{
	if (!base) [[unlikely]] {
		throw Exception("Unsupported operation on EntityDataAppliedDelta", HalleyExceptions::Entity);
	}
	
	if (delta) {
		if (const auto& uuid = delta->getPrefabUUID()) {
			return *uuid;
		}
	}
	return base->getPrefabUUID();
}

const UUID& EntityDataAppliedDelta::getParentUUID() const
{
	if (!base) [[unlikely]] {
		throw Exception("Unsupported operation on EntityDataAppliedDelta", HalleyExceptions::Entity);
	}
	
	if (delta) {
		if (const auto& uuid = delta->getParentUUID()) {
			return *uuid;
		}
	}
	return base->getParentUUID();
}

size_t EntityDataAppliedDelta::getNumChildren() const
{
	// TODO
	return 0;
}

const IEntityConcreteData& EntityDataAppliedDelta::getChild(size_t idx, EntityDataAppliedDelta& buffer) const
{
	// TODO
	return buffer;
}

size_t EntityDataAppliedDelta::getNumComponents() const
{
	// TODO
	return 0;
}

std::tuple<std::string_view, const ConfigNode*, const ConfigNode*> EntityDataAppliedDelta::getComponentAndDelta(size_t idx) const
{
	// TODO
	return {};
}
