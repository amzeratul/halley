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

Time SharedData::getTimeSinceLastSend() const
{
	return timeSinceLastSend;
}

void SharedData::markSent()
{
	timeSinceLastSend = 0;
}

void SharedData::update(Time t)
{
	timeSinceLastSend += t;
}

void SharedData::serialize(Serializer& s) const
{
}

void SharedData::deserialize(Deserializer& s)
{
}
