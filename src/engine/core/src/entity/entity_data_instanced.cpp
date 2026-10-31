#include "halley/entity/entity_data_instanced.h"
using namespace Halley;

EntityDataInstanced::EntityDataInstanced(const EntityData& prefabData, const IEntityConcreteData& instanceData, bool allowAddComponensNotInPrefab)
	: prefabData(&prefabData)
{
	instanceUUID = instanceData.getInstanceUUID();
	flags = instanceData.getFlags();
	variant = instanceData.getVariant();
	enableRules = instanceData.getEnableRules();

	children.reserve(prefabData.getNumChildren());
	for (const auto& child: prefabData.getChildren()) {
		children.emplace_back(EntityDataInstanced(child, instanceUUID));
	}

	const auto& nComponents = instanceData.getNumComponents();
	for (size_t i = 0; i < nComponents; ++i) {
		ConfigNode buffer;
		const auto& component = instanceData.getComponentApplied(i, buffer);
		if (component.second) {
			if (prefabData.tryGetComponent(component.first)) {
				componentOverrides[component.first] = std::pair{ component.first, *component.second };
			} else if (allowAddComponensNotInPrefab) {
				componentsAdded.emplace_back(std::pair{ component.first, *component.second });
			}
		}
	}
}

EntityDataInstanced::EntityDataInstanced(const EntityData& prefabData, const UUID& rootInstanceUUID)
	: prefabData(&prefabData)
{
	instanceUUID = UUID::xorUUIDs(prefabData.getPrefabUUID(), rootInstanceUUID);

	children.reserve(prefabData.getNumChildren());
	for (const auto& child: prefabData.getChildren()) {
		children.emplace_back(EntityDataInstanced(child, rootInstanceUUID));
	}
}

IEntityData::Type EntityDataInstanced::getType() const
{
	return Type::Instanced;
}

const String& EntityDataInstanced::getName() const
{
	return prefabData->getName();
}

const String& EntityDataInstanced::getPrefab() const
{
	return prefabData->getPrefab();
}

const String& EntityDataInstanced::getVariant() const
{
	return variant.isEmpty() ? prefabData->getVariant() : variant;
}

const String& EntityDataInstanced::getEnableRules() const
{
	return enableRules.isEmpty() ? prefabData->getEnableRules() : enableRules;
}

uint8_t EntityDataInstanced::getFlags() const
{
	return prefabData->getFlags() | flags;
}

bool EntityDataInstanced::getFlag(Flag flag) const
{
	if ((flags & static_cast<uint8_t>(flag)) != 0) {
		return true;
	}
	return prefabData->getFlag(flag);
}

const UUID& EntityDataInstanced::getInstanceUUID() const
{
	return instanceUUID;
}

const UUID& EntityDataInstanced::getPrefabUUID() const
{
	return prefabData->getPrefabUUID();
}

size_t EntityDataInstanced::getNumChildren() const
{
	return prefabData->getNumChildren();
}

const IEntityConcreteData& EntityDataInstanced::getChild(size_t idx, EntityDataAppliedDelta& buffer) const
{
	return children[idx];
}

size_t EntityDataInstanced::getNumComponents() const
{
	return prefabData->getNumComponents() + componentsAdded.size();
}

std::tuple<std::string_view, const ConfigNode*, const ConfigNode*> EntityDataInstanced::getComponentAndDelta(size_t idx) const
{
	const size_t numComponentsInPrefab = prefabData->getNumComponents();

	if (idx < numComponentsInPrefab) {
		const auto& original = prefabData->getComponent(idx);
		const auto iter = componentOverrides.find(original.first);

		if (iter != componentOverrides.end()) {
			const auto& fromData = original.second;
			const auto& toData = iter->second.second;
			if (toData.asMap().empty()) {
				return { original.first, &original.second, nullptr };
			}

			if (toData.getType() == ConfigNodeType::DeltaMap && !fromData.asMap().empty()) {
				return { original.first, &original.second, &toData };
			} else {
				return { original.first, &toData, nullptr };
			}
		}
		return { original.first, &original.second, nullptr };
	} else {
		const auto& comp = componentsAdded.at(idx - numComponentsInPrefab);
		return { comp.first, &comp.second, nullptr };
	}
}
