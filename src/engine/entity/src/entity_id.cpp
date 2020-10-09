#include "entity_id.h"

#include "world.h"
#include "halley/maths/uuid.h"
#include "halley/entity/entity_factory.h"
#include "halley/support/logger.h"

using namespace Halley;

String EntityId::toUUID(const EntityId& id, ConfigNodeSerializationContext& context)
{
	auto& world = context.entityContext->world;
	return world.getEntity(id).getInstanceUUID().toString();
}

EntityId EntityId::fromUUID(const String& uuidStr, ConfigNodeSerializationContext& context)
{
	const auto iter = context.entityContext->uuids.find(UUID(uuidStr));
	if (iter != context.entityContext->uuids.end()) {
		return iter->second;
	}

	const auto& prefabIter = context.entityContext->uuidMapping.back().find(UUID(uuidStr));	
	if (prefabIter != context.entityContext->uuidMapping.back().end()) {
		const auto& instanceIter = context.entityContext->uuids.find(prefabIter->second);
		if (instanceIter != context.entityContext->uuids.end()) {
			return instanceIter->second;
		}
	}
	
	return EntityId();
}
