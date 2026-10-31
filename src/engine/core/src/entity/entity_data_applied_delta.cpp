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

	if (delta) {
		const auto& changed = delta->getComponentsChanged();
		for (size_t i = 0; i < changed.size(); ++i) {
			if (!base.hasComponent(changed[i].first)) {
				componentsAdded += static_cast<int16_t>(i);
			}
		}
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
	if (base && delta) {
		const auto nBaseChildren = base->getNumChildren();
        const auto nAdded = delta->getChildrenAdded().size();
		return nBaseChildren + nAdded;
	} else if (base) {
		return base->getNumChildren();
	} else {
		return 0;
	}
}

const IEntityConcreteData* EntityDataAppliedDelta::getChild(size_t idx, EntityDataAppliedDelta& buffer) const
{
	HalleyAssertDev(base);

	const auto nBaseChildren = base->getNumChildren();
	if (idx >= nBaseChildren) {
		return &delta->getChildrenAdded().at(idx - nBaseChildren);
	}

	const auto& orig = base->getChild(idx);
	const EntityDataDelta* childDelta = nullptr;

	if (delta) {
		const auto& prefabUUID = orig.getPrefabUUID();
		const auto instancedUUID = UUID::xorUUIDs(rootUUID, prefabUUID);

		const auto& removed = delta->getChildrenRemoved();
		if (removed.contains(prefabUUID) || removed.contains(instancedUUID)) {
			return nullptr;
		}

		for (const auto& [k, d]: delta->getChildrenChanged()) {
			if (k == prefabUUID || k == instancedUUID) {
				childDelta = &d;
				break;
			}
		}
	}

	buffer = EntityDataAppliedDelta(orig, childDelta, rootUUID, false);
	return &buffer;
}

size_t EntityDataAppliedDelta::getNumComponents() const
{
	if (base && delta) {
		const auto nBase = base->getNumComponents();
		const auto nAdded = componentsAdded.size();
		return nBase + nAdded;
	} else if (base) {
		return base->getNumComponents();
	} else {
		return 0;
	}
}

std::tuple<std::string_view, const ConfigNode*, const ConfigNode*> EntityDataAppliedDelta::getComponentAndDelta(size_t idx) const
{
	HalleyAssertDev(base);

	if (delta) {
		const auto nBase = base->getNumComponents();
		if (idx >= nBase) {
			// Added component
			const auto& comp = delta->getComponentsChanged().at(componentsAdded.at(idx - nBase));
			return { comp.first, nullptr, &comp.second };
		}

		const auto& [name, baseComp] = base->getComponent(idx);

		if (delta->getComponentsRemoved().contains(name)) {
			return { {}, nullptr, nullptr };
		}

		for (const auto& [k, d]: delta->getComponentsChanged()) {
			if (k == name) {
				return { name, &baseComp, &d };
			}
		}

		return { name, &baseComp, nullptr };
	} else {
		return base->getComponentAndDelta(idx);
	}
}

bool EntityDataAppliedDelta::hasChildWithUUID(const UUID& uuid) const
{
	/*
	if (base && delta) {
		return (base->hasChildWithUUID(uuid) && !delta->getChildrenRemoved().contains(uuid))
			|| (delta->getChildrenAdded().find_if([&] (const EntityData& d) { return d.getInstanceUUID() == uuid; }) != delta->getChildrenAdded().end());
	} else if (base) {
		return base->hasChildWithUUID(uuid);
	} else {
		return false;
	}
	*/
	return IEntityConcreteData::hasChildWithUUID(uuid);
}
