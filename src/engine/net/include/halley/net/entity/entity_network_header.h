#pragma once

#include <cstdint>

namespace Halley {
    using EntityNetworkId = uint16_t;

    enum class EntityNetworkHeaderType : uint8_t {
        Create,
        Update,
        Destroy,
    	ReadyToStart
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
}
