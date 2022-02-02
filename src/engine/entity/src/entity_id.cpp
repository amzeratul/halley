#include "entity_id.h"

#include "world.h"
#include "halley/maths/uuid.h"
#include "halley/entity/entity_factory.h"
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

String EntityId::toUUID(const EntityId& id, const EntitySerializationContext& context)
{
	if (!context.entityContext) {
		return "";
	}
	auto& world = context.entityContext->getWorld();
	return world.getEntity(id).getInstanceUUID().toString();
}

EntityId EntityId::fromUUID(const String& uuidStr, const EntitySerializationContext& context)
{
	if (!context.entityContext) {
		return EntityId();
	}
	return context.entityContext->getEntityIdFromUUID(UUID(uuidStr));
}
