#pragma once
#include "halley/text/enum_names.h"
#include "halley/utils/utils.h"
#include "halley/bytes/byte_serializer.h"

namespace Halley {
	enum class NetworkSessionType : uint8_t {
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

	enum class NetworkSessionMessageType : uint8_t {
		Control,
		ToAllPeers,
		ToPeer
	};

	template <>
	struct EnumNames<NetworkSessionMessageType> {
		constexpr std::array<const char*, 3> operator()() const {
			return{{
				"control",
				"toAllPeers",
				"toPeer"
			}};
		}
	};

	struct NetworkSessionMessageHeader {
		NetworkSessionMessageType type;
		uint8_t srcPeerId;
		uint8_t dstPeerId;
	};
}
