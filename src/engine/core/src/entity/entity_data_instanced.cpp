#include "halley/entity/entity_data_instanced.h"
using namespace Halley;

EntityDataInstanced::EntityDataInstanced(const EntityData& prefabData, const IEntityConcreteData& instanceData, bool allowAddComponensNotInPrefab)
	: EntityDataInstanced(prefabData, instanceData, allowAddComponensNotInPrefab, instanceData.getInstanceUUID())
{
}

EntityDataInstanced::EntityDataInstanced(const EntityData& prefabData, const IEntityConcreteData& instanceData, bool allowAddComponensNotInPrefab, const UUID& rootInstanceUUID)
	: prefabData(&prefabData)
{
	instanceUUID = instanceData.getInstanceUUID();
	flags = instanceData.getFlags();
	variant = instanceData.getVariant();
	enableRules = instanceData.getEnableRules();

	const auto* instanceEntityData = allowAddComponensNotInPrefab && instanceData.getType() == Type::Data ? static_cast<const EntityData*>(&instanceData) : nullptr;

	auto findChild = [](const Vector<EntityData>& list, const UUID& prefabUUID) -> const EntityData* {
		for (const auto& c: list) {
			if (c.getPrefabUUID().isValid() && c.getPrefabUUID() == prefabUUID) {
				return &c;
			}
		}
		return nullptr;
	};

	children.reserve(prefabData.getNumChildren());
	for (const auto& child: prefabData.getChildren()) {
		const auto* instanceChild = instanceEntityData ? findChild(instanceEntityData->getChildren(), child.getPrefabUUID()) : nullptr;
		if (instanceChild) {
			children.emplace_back(EntityDataInstanced(child, *instanceChild, true, rootInstanceUUID));
		} else {
			children.emplace_back(EntityDataInstanced(child, rootInstanceUUID));
		}
	}

	if (instanceEntityData) {
		// Children not in prefab
		for (const auto& c: instanceEntityData->getChildren()) {
			if (!findChild(prefabData.getChildren(), c.getPrefabUUID())) {
				childrenAdded.push_back(&c);
			}
		}
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

const UUID& EntityDataInstanced::getParentUUID() const
{
	throw Exception("Unimplemented", HalleyExceptions::Entity);
}

size_t EntityDataInstanced::getNumChildren() const
{
	return children.size() + childrenAdded.size();
}

const IEntityConcreteData* EntityDataInstanced::getChild(size_t idx, EntityDataAppliedDelta& buffer) const
{
	if (idx < children.size()) {
		return &children[idx];
	}
	return childrenAdded[idx - children.size()];
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
