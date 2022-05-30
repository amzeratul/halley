#include "entity_id_serializer.h"
#include "world.h"
#include "halley/maths/uuid.h"
#include "halley/entity/entity_factory.h"
using namespace Halley;

ConfigNode ConfigNodeSerializer<EntityId>::serialize(EntityId id, const EntitySerializationContext& context)
{
	if (!context.entityContext) {
		return ConfigNode(String());
	}
	auto& world = context.entityContext->getWorld();
	return ConfigNode(world.getEntity(id).getInstanceUUID().toString());
}

EntityId ConfigNodeSerializer<EntityId>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	if (!context.entityContext) {
		return EntityId();
	}
	return context.entityContext->getEntityIdFromUUID(UUID(node.asString("")));
}
