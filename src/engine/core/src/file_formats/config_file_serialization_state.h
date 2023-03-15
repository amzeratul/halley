#pragma once

#include "halley/bytes/byte_serializer.h"

namespace Halley {
	class ConfigFileSerializationState : public SerializerState {
	public:
		bool storeFilePosition = false;
	};
}
