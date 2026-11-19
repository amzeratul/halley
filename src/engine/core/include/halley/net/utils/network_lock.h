#pragma once

#include <cstdint>
#include "halley/bytes/config_node_serializer_base.h"
#include "halley/bytes/byte_serializer.h"

namespace Halley {
    // From least to most strict
    enum class NetworkLockType : uint8_t {
        Shared,
        Unique,
        UniqueWithAuthority
    };

    struct NetworkLockInfo {
        uint8_t peerId;
        NetworkLockType type;

        NetworkLockInfo(uint8_t peerId = 0, NetworkLockType type = NetworkLockType::Shared)
	        : peerId(peerId)
			, type(type)
		{}

        explicit NetworkLockInfo(const ConfigNode& node)
        {
	        const auto i = node.asInt();
            peerId = uint8_t((i >> 8) & 0xFF);
        	type = NetworkLockType(i & 0xFF);
        }

        ConfigNode toConfigNode() const
        {
	        return ConfigNode(int(peerId) << 8 | int(type));
        }

        void serialize(Serializer& s) const
        {
	        s << peerId;
            s << type;
        }

        void deserialize(Deserializer& s)
        {
	        s >> peerId;
            s >> type;
        }

        bool operator==(const NetworkLockInfo& other) const = default;
    };

    
	template<>
	class ConfigNodeSerializer<NetworkLockInfo> {
	public:
        ConfigNode serialize(const NetworkLockInfo& info, const EntitySerializationContext& context)
        {
            return info.toConfigNode();
        }

        NetworkLockInfo deserialize(const EntitySerializationContext& context, const ConfigNode& node)
        {
            return NetworkLockInfo(node);
        }
	};
}
