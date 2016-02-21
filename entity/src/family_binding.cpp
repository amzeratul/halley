#include "family_binding.h"
#include "family.h"

using namespace Halley;

FamilyBindingBase::FamilyBindingBase(FamilyMaskType readMask, FamilyMaskType writeMask)
	: family(nullptr)
	, readMask(readMask)
	, writeMask(writeMask)
{
}

void FamilyBindingBase::setFamily(Family* f) {
	family = f;
}

size_t FamilyBindingBase::count() const {
	return family->count();
}

void* FamilyBindingBase::getElement(size_t index) const {
	return family->getElement(index);
}
