#include "entity_id.h"

#include "world.h"
#include "halley/maths/uuid.h"
#include "halley/entity/entity_factory.h"

using namespace Halley;

String EntityId::toUUID(const EntityId& id, ConfigNodeSerializationContext& context)
{
	auto& world = context.entityContext->world;
	return world.getEntity(id).getUUID().toString();
}

EntityId EntityId::fromUUID(const String& uuidStr, ConfigNodeSerializationContext& context)
{
	const auto iter = context.entityContext->uuids.find(UUID(uuidStr));
	if (iter != context.entityContext->uuids.end()) {
		return iter->second;
	}
	return EntityId();
}
