#include "../include/halley/entity/family.h"

using namespace Halley;

Family::Family(FamilyMaskType mask) 
	: inclusionMask(mask)
{}

void Family::removeEntity(Entity& entity)
{
	toRemove.push_back(entity.getEntityId());
}
