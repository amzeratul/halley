#pragma once
#include "../utils/utils.h"
#include <gsl/gsl>
#include <limits>

namespace Halley {
	class Compression {
	public:
		static Bytes deflate(const Bytes& bytes);
		static Bytes deflate(gsl::span<const gsl::byte> bytes);
		static Bytes inflate(const Bytes& bytes, size_t maxSize = std::numeric_limits<size_t>::max());
		static Bytes inflate(gsl::span<const gsl::byte> bytes, size_t maxSize = std::numeric_limits<size_t>::max());
		static std::shared_ptr<const char> inflateToSharedPtr(gsl::span<const gsl::byte> bytes, size_t& outSize, size_t maxSize = std::numeric_limits<size_t>::max());
		static unsigned char* inflateRaw(gsl::span<const gsl::byte> bytes, size_t& outSize, size_t maxSize);
	};
}
