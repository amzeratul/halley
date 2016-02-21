#include "entity.h"
#include "type_deleter.h"
//#include "../../exception.h"

using namespace Halley;

Entity::Entity()
	: mask(0)
	, uid(-1)
	, dirty(false)
	, alive(true)
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
	dirty = true;
	if (fastId >= 0) {
		fastComponents[fastId] = component;
	}
}

void Entity::deleteComponent(Component* component, int id, int fastId)
{
	TypeDeleterBase* deleter = TypeUIDSizeTable::get(id);
	deleter->callDestructor(component);
	PoolPool::getPool(deleter->getSize())->free(component);
	if (fastId >= 0) {
		fastComponents[fastId] = nullptr;
	}
}

FamilyMaskType Entity::getMask() const
{
	return mask;
}

void Entity::refresh()
{
	if (dirty) {
		mask = 0;
		dirty = false;
		for (auto i : components) {
			mask = FamilyMask::make(i.first, mask);
		}
	}
}

EntityId Entity::getUID() const
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
