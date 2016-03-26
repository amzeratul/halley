#include "family_mask.h"

using namespace Halley;

FamilyMask::Handle::Handle()
	: value(0)
{
}

FamilyMask::Handle::Handle(const Handle& h)
	: value(h.value)
{
}

FamilyMask::Handle::Handle(Handle&& h)
	: value(h.value)
{
}

FamilyMask::Handle::Handle(const RealType& mask)
	: value(mask)
{
}

FamilyMask::Handle::Handle(RealType&& mask)
	: value(mask)
{
}

void FamilyMask::Handle::operator=(const Handle& h)
{
	value = h.value;
}

bool FamilyMask::Handle::operator==(const Handle& h) const
{
	return value == h.value;
}

bool FamilyMask::Handle::operator!=(const Handle& h) const
{
	return value != h.value;
}

bool FamilyMask::Handle::operator<(const Handle& h) const
{
	return value < h.value;
}

FamilyMask::Handle FamilyMask::Handle::operator&(const Handle& h) const
{
	return Handle(value & h.value);
}

const FamilyMask::RealType& FamilyMask::Handle::getRealValue() const
{
	return value;
}

FamilyMask::HandleType FamilyMask::getHandle(RealType mask)
{
	return Handle(mask);
}
