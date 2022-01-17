#pragma once
#include "halley/utils/utils.h"
#include "halley/bytes/byte_serializer.h"

namespace Halley {
	enum class NetworkSessionType {
		Undefined,
		Host,
		Client
	};

	template <>
	struct EnumNames<NetworkSessionType> {
		constexpr std::array<const char*, 3> operator()() const {
			return{{
				"undefined",
				"host",
				"client"
			}};
		}
	};

	enum class NetworkSessionMessageType : char {
		Control,
		ToPeers,
		ToMaster
	};

	template <>
	struct EnumNames<NetworkSessionMessageType> {
		constexpr std::array<const char*, 3> operator()() const {
			return{{
				"control",
				"toPeers",
				"toMaster"
			}};
		}
	};

	struct NetworkSessionMessageHeader {
		NetworkSessionMessageType type;
		uint8_t srcPeerId;
	};
}
