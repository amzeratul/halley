#include "session/shared_data.h"
using namespace Halley;

void SharedData::markModified()
{
	modified = true;
}

void SharedData::markUnmodified()
{
	modified = false;
}

bool SharedData::isModified() const
{
	return modified;
}
