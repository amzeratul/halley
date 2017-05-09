#pragma once
#include "halley/utils/utils.h"
#include "halley/file/byte_serializer.h"

namespace Halley {
	enum class NetworkSessionControlMessageType : char {
		SetPeerId
	};

	struct ControlMsgHeader
	{
		NetworkSessionControlMessageType type;
	};

	struct ControlMsgSetPeerId
	{
		char peerId;
	};
}
