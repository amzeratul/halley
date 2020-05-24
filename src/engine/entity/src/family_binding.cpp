#include "family_binding.h"

using namespace Halley;

FamilyBindingBase::~FamilyBindingBase()
{
	if (addedCallback) {
		family->removeOnEntityAdded(this);
	}
	if (removedCallback) {
		family->removeOnEntityRemoved(this);
	}
}

void FamilyBindingBase::doInit(FamilyMaskType read, FamilyMaskType write) noexcept
{
	readMask = read;
	writeMask = write;
}

void FamilyBindingBase::onEntitiesAdded(void* entity, size_t count)
{
	addedCallback(entity, count);
}

void FamilyBindingBase::onEntitiesRemoved(void* entity, size_t count)
{
	removedCallback(entity, count);
}

void FamilyBindingBase::setFamily(Family* f) noexcept {
	family = f;
}

void FamilyBindingBase::setOnEntitiesAdded(std::function<void(void*, size_t)> callback)
{
	addedCallback = callback;
	family->addOnEntitiesAdded(this);
}

void FamilyBindingBase::setOnEntitiesRemoved(std::function<void(void*, size_t)> callback)
{
	removedCallback = callback;
	family->addOnEntitiesRemoved(this);
}
