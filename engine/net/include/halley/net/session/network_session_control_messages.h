#pragma once
#include "halley/utils/utils.h"
#include "halley/file/byte_serializer.h"

namespace Halley {
	enum class NetworkSessionControlMessageType : char {
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
		char peerId;
	};

	struct ControlMsgSetSessionState {
		Bytes state;
	};

	struct ControlMsgSetPeerState {
		char peerId;
		Bytes state;
	};
}
