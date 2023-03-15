#pragma once

#include "utils.h"

namespace Halley {
	class String;

	class Encrypt {
	public:
		static Bytes encrypt(const Bytes& iv, const String& key, const Bytes& data);
		static Bytes decrypt(const Bytes& iv, const String& key, const Bytes& data);
	};
}