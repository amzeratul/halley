#include "family.h"

using namespace Halley;

Family::Family(FamilyMask::Type mask) 
	: inclusionMask(mask)
{}

void Family::removeEntity(Entity& entity)
{
	toRemove.push_back(entity.getUID());
}
