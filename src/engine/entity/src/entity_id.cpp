#include "entity_id.h"
#include "halley/maths/uuid.h"
#include "halley/entity/entity_factory.h"

using namespace Halley;

EntityId EntityId::fromUUID(const String& uuidStr, ConfigNodeSerializationContext& context)
{
	auto iter = context.entityContext->uuids.find(UUID(uuidStr));
	if (iter != context.entityContext->uuids.end()) {
		return iter->second;
	}
	return EntityId();
}
