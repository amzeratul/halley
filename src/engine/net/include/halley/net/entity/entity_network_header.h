#pragma once

#include <cstdint>

#include "halley/maths/uuid.h"

namespace Halley {
    using EntityNetworkId = uint16_t;

    enum class EntityNetworkHeaderType : uint8_t {
        Create,
        Update,
        Destroy,
    	ReadyToStart,
    	MessageToEntity,
    	MessageToSystem, // Not implemented
    	RPC, // Not implemented
    };
    
    struct EntityNetworkHeader {
        EntityNetworkHeaderType type;

        EntityNetworkHeader() = default;
        EntityNetworkHeader(EntityNetworkHeaderType type) : type(type) {}
    };

	struct EntityNetworkEntityHeader {
        EntityNetworkId entityId;

        EntityNetworkEntityHeader() = default;
        EntityNetworkEntityHeader(EntityNetworkId entityId) : entityId(entityId) {}
	};

	struct EntityNetworkMessageToEntityHeader {
        UUID entityUUID;
        int messageType = 0;

        EntityNetworkMessageToEntityHeader() = default;
        EntityNetworkMessageToEntityHeader(UUID uuid, int messageType) : entityUUID(uuid), messageType(messageType) {}
	};
}
