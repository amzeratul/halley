#include <halley/data_structures/memory_pool.h>
#include "../include/halley/entity/entity.h"
#include "../include/halley/entity/world.h"

using namespace Halley;

Entity::Entity()
{
	for (size_t i = 0; i < numFastComponents; i++) {
		fastComponents[i] = nullptr;
	}
}

Entity::~Entity()
{
	for (auto i = components.begin(); i != components.end(); i++) {
		deleteComponent(i->second, i->first, -1);
	}
}

void Entity::addComponent(Component* component, int id, int fastId)
{
	components.push_back(std::pair<int, Component*>(id, component));
	if (fastId >= 0) {
		fastComponents[fastId] = component;
	}
}

void Entity::deleteComponent(Component* component, int id, int fastId)
{
	TypeDeleterBase* deleter = ComponentDeleterTable::get(id);
	deleter->callDestructor(component);
	PoolPool::getPool(deleter->getSize())->free(component);
	if (fastId >= 0) {
		fastComponents[fastId] = nullptr;
	}
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

FamilyMaskType Entity::getMask() const
{
	return mask;
}

void Entity::refresh()
{
	if (dirty) {
		auto m = FamilyMask::RealType();
		dirty = false;
		for (auto i : components) {
			FamilyMask::setBit(m, i.first);
		}
		mask = FamilyMask::getHandle(m);
	}
}

EntityId Entity::getEntityId() const
{
	if (uid == -1) {
		//throw Exception("Entity ID not yet assigned - are you using this before it's spawned?");
	}
	return uid;
}

void Entity::destroy()
{
	alive = false;
	dirty = true;
}
