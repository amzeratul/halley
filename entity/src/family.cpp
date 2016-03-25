#include "family.h"

using namespace Halley;

Family::Family(FamilyMask::Type read, FamilyMask::Type write) 
	: inclusionMask(read | write)
	, writeMask(write)
	, readMask(read)
{}

FamilyMask::Type Family::getInclusionMask() const
{
	return inclusionMask;
}

FamilyMask::Type Family::getMutableMask() const
{
	return writeMask;
}

FamilyMask::Type Family::getConstMask() const
{
	return readMask;
}
