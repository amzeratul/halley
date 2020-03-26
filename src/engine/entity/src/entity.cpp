#include <halley/data_structures/memory_pool.h>
#include "entity.h"
#include "world.h"

using namespace Halley;

Entity::Entity() = default;
Entity::~Entity() = default;

void Entity::destroyComponents(ComponentDeleterTable& table)
{
	for (auto& component : components) {
		deleteComponent(component.second, component.first, table);
	}
	components.clear();
	liveComponents = 0;
}

void Entity::addComponent(Component* component, int id)
{
	components.push_back(std::pair<int, Component*>(id, component));
	if (liveComponents < int(components.size())) {
		// Swap with first non-live component
		std::swap(components[liveComponents], components.back());
	}
	++liveComponents;
}

void Entity::removeComponentAt(int i)
{
	// Put it at the end and decrease live count
	std::swap(components[i], components.back());
	--liveComponents;
}

void Entity::deleteComponent(Component* component, int id, ComponentDeleterTable& table)
{
	TypeDeleterBase* deleter = table.get(id);
	deleter->callDestructor(component);
	PoolPool::getPool(deleter->getSize())->free(component);
}

void Entity::onReady()
{
}

void Entity::markDirty(World& world)
{
	if (!dirty) {
		dirty = true;
		world.onEntityDirty();
	}
}

ComponentDeleterTable& Entity::getComponentDeleterTable(World& world)
{
	return world.getComponentDeleterTable();
}

FamilyMaskType Entity::getMask() const
{
	return mask;
}

void Entity::refresh(MaskStorage& storage, ComponentDeleterTable& table)
{
	if (dirty) {
		dirty = false;

		// Delete stale components
		for (int i = liveComponents; i < int(components.size()); ++i) {
			deleteComponent(components[i].second, components[i].first, table);
		}
		components.resize(liveComponents);

		// Re-generate mask
		auto m = FamilyMask::RealType();
		for (auto i : components) {
			FamilyMask::setBit(m, i.first);
		}
		mask = FamilyMask::getHandle(m, storage);
	}
}

EntityId Entity::getEntityId() const
{
	if (!uid.isValid()) {
		//throw Exception("Entity ID not yet assigned - are you using this before it's spawned?");
	}
	return uid;
}

void Entity::destroy()
{
	alive = false;
	dirty = true;
}
