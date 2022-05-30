#include "entity_id.h"

namespace Halley {
	template <>
    class ConfigNodeSerializer<EntityId> {
    public:
		ConfigNode serialize(EntityId id, const EntitySerializationContext& context);
		EntityId deserialize(const EntitySerializationContext& context, const ConfigNode& node);
    };
}
