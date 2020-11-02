#include "entity_id.h"

#include "world.h"
#include "halley/maths/uuid.h"
#include "halley/entity/entity_factory.h"

using namespace Halley;

String EntityId::toUUID(const EntityId& id, const ConfigNodeSerializationContext& context)
{
	auto& world = context.entityContext->getWorld();
	return world.getEntity(id).getInstanceUUID().toString();
}

EntityId EntityId::fromUUID(const String& uuidStr, const ConfigNodeSerializationContext& context)
{
	return context.entityContext->getEntityIdFromUUID(UUID(uuidStr));
}
