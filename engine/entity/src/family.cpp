#include "family.h"
#include "family_binding.h"

using namespace Halley;

Family::Family(FamilyMaskType mask) 
	: inclusionMask(mask)
{}

void Family::addOnEntityAdded(FamilyBindingBase* bind)
{
	addEntityCallbacks.push_back(bind);
}

void Family::removeOnEntityAdded(FamilyBindingBase* bind)
{
	addEntityCallbacks.erase(std::remove(addEntityCallbacks.begin(), addEntityCallbacks.end(), bind), addEntityCallbacks.end());
}

void Family::addOnEntityRemoved(FamilyBindingBase* bind)
{
	removeEntityCallbacks.push_back(bind);
}

void Family::removeOnEntityRemoved(FamilyBindingBase* bind)
{
	removeEntityCallbacks.erase(std::remove(removeEntityCallbacks.begin(), removeEntityCallbacks.end(), bind), removeEntityCallbacks.end());
}

void Family::notifyAdd(void* entity)
{
	for (auto& c: addEntityCallbacks) {
		c->onEntityAdded(entity);
	}
}

void Family::notifyRemove(void* entity)
{
	for (auto& c: removeEntityCallbacks) {
		c->onEntityRemoved(entity);
	}	
}

void Family::removeEntity(Entity& entity)
{
	toRemove.push_back(entity.getEntityId());
}
