#pragma once
#include "../utils/utils.h"
#include <gsl/gsl>
#include <limits>

namespace Halley {
	class Compression {
	public:
		static Bytes compress(const Bytes& bytes);
		static Bytes compress(gsl::span<const gsl::byte> bytes);
		static Bytes decompress(const Bytes& bytes, size_t maxSize = std::numeric_limits<size_t>::max());
		static Bytes decompress(gsl::span<const gsl::byte> bytes, size_t maxSize = std::numeric_limits<size_t>::max());
		static std::shared_ptr<const char> decompressToSharedPtr(gsl::span<const gsl::byte> bytes, size_t& outSize, size_t maxSize = std::numeric_limits<size_t>::max());

		static Bytes compressRaw(gsl::span<const gsl::byte> bytes, bool insertLength);
		static Bytes decompressRaw(gsl::span<const gsl::byte> bytes, size_t maxSize, size_t expectedSize = 0);
	};
}
