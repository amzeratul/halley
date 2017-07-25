#pragma once
#include "../utils/utils.h"
#include <gsl/gsl>

namespace Halley {
	class Compression {
	public:
		static Bytes deflate(gsl::span<const gsl::byte> bytes);
		static Bytes inflate(gsl::span<const gsl::byte> bytes);
		static std::shared_ptr<const char> inflateRaw(gsl::span<const gsl::byte> bytes, size_t& outSize);
	};
}
