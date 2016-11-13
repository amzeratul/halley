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

FamilyBindingBase::FamilyBindingBase(FamilyMaskType readMask, FamilyMaskType writeMask)
	: family(nullptr)
	, readMask(readMask)
	, writeMask(writeMask)
{
}

void FamilyBindingBase::onEntityAdded(void* entity)
{
	addedCallback(entity);
}

void FamilyBindingBase::onEntityRemoved(void* entity)
{
	removedCallback(entity);
}

void FamilyBindingBase::setFamily(Family* f) {
	family = f;
}

void FamilyBindingBase::setOnEntityAdded(std::function<void(void*)> callback)
{
	addedCallback = callback;
	family->addOnEntityAdded(this);
}

void FamilyBindingBase::setOnEntityRemoved(std::function<void(void*)> callback)
{
	removedCallback = callback;
	family->addOnEntityRemoved(this);
}
