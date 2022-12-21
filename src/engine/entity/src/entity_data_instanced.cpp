#include "entity_data_instanced.h"
using namespace Halley;

EntityDataInstanced::EntityDataInstanced(const EntityData& prefabData, const IEntityConcreteData& instanceData)
	: prefabData(&prefabData)
{
	instanceUUID = instanceData.getInstanceUUID();
	flags = instanceData.getFlags();

	children.reserve(prefabData.getNumChildren());
	for (const auto& child: prefabData.getChildren()) {
		children.emplace_back(EntityDataInstanced(child, instanceUUID));
	}

	const auto& nComponents = instanceData.getNumComponents();
	for (size_t i = 0; i < nComponents; ++i) {
		const auto& component = instanceData.getComponent(i);
		componentOverrides[component.first] = component;
	}
}

EntityDataInstanced::EntityDataInstanced(const EntityData& prefabData, const UUID& rootInstanceUUID)
	: prefabData(&prefabData)
{
	instanceUUID = UUID::generateFromUUIDs(prefabData.getPrefabUUID(), rootInstanceUUID);

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

const IEntityConcreteData& EntityDataInstanced::getChild(size_t idx) const
{
	return children[idx];
}

size_t EntityDataInstanced::getNumComponents() const
{
	return prefabData->getNumComponents();
}

const std::pair<String, ConfigNode>& EntityDataInstanced::getComponent(size_t idx) const
{
	const auto& original = prefabData->getComponent(idx);
	const auto iter = componentOverrides.find(original.first);
	if (iter != componentOverrides.end()) {
		return iter->second;
	}
	return original;
}
