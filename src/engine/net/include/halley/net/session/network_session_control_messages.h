#pragma once
#include "halley/utils/utils.h"
#include "halley/file/byte_serializer.h"

namespace Halley {
	enum class NetworkSessionControlMessageType : int8_t {
		SetPeerId,
		SetSessionState,
		SetPeerState
	};

	struct ControlMsgHeader
	{
		NetworkSessionControlMessageType type;
	};

	struct ControlMsgSetPeerId
	{
		int8_t peerId;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	struct ControlMsgSetSessionState {
		Bytes state;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	struct ControlMsgSetPeerState {
		int8_t peerId;
		Bytes state;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};
}
