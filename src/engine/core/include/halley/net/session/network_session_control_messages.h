#pragma once
#include "halley/utils/utils.h"
#include "halley/bytes/byte_serializer.h"

namespace Halley {
	enum class NetworkSessionControlMessageType : int8_t {
		Join,
		SetPeerId,
		SetSessionState,
		SetPeerState,
		SetServerSideData,
		SetServerSideDataReply,
		GetServerSideData,
		GetServerSideDataReply
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
		uint8_t peerId = 0;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	struct ControlMsgSetSessionState {
		Bytes state;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	struct ControlMsgSetPeerState {
		uint8_t peerId = 0;
		Bytes state;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	struct ControlMsgSetServerSideData {
		String key;
		ConfigNode data;
		uint32_t requestId;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	struct ControlMsgSetServerSideDataReply {
		bool ok;
		uint32_t requestId;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	struct ControlMsgGetServerSideData {
		String key;
		uint32_t requestId;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	struct ControlMsgGetServerSideDataReply {
		ConfigNode data;
		uint32_t requestId;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};
}
