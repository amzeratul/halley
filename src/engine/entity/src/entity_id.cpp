#include "entity_id.h"

#include "world.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/text/string_converter.h"
#include "halley/text/halleystring.h"

using namespace Halley;

EntityId::EntityId(const String& str)
{
	value = str.toInteger64();
}

String EntityId::toString() const
{
	return Halley::toString(value);
}

void EntityId::serialize(Serializer& s) const
{
	auto* world = s.getOptions().world;
	if (world) {
		auto e = world->getEntity(*this);
		if (e.isValid()) {
			s << e.getInstanceUUID();
		} else {
			s << UUID();
		}
	} else {
		throw Exception("Serializing EntityID requires World to be set in SerializationOptions.", HalleyExceptions::Entity);
	}
}

void EntityId::deserialize(Deserializer& s)
{
	auto* world = s.getOptions().world;
	if (world) {
		UUID uuid;
		s >> uuid;
		
		auto e = world->findEntity(uuid);
		if (e) {
			*this = e->getEntityId();
		} else {
			*this = EntityId();
		}
	} else {
		throw Exception("Deserializing EntityID requires World to be set in SerializationOptions.", HalleyExceptions::Entity);
	}
}
