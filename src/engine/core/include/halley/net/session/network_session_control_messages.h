#pragma once
#include "halley/utils/utils.h"
#include "halley/bytes/byte_serializer.h"

namespace Halley {
	enum class NetworkSessionControlMessageType : int8_t {
		Join,
		SetPeerId,
		SetSessionState,
		SetPeerState
	};

	struct ControlMsgHeader
	{
		NetworkSessionControlMessageType type;
	};

	struct ControlMsgJoin {
		uint32_t networkVersion;
		String userName;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	struct ControlMsgSetPeerId {
		int8_t peerId = 0;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	struct ControlMsgSetSessionState {
		Bytes state;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	struct ControlMsgSetPeerState {
		int8_t peerId = 0;
		Bytes state;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};
}
