#pragma once
#include "../utils/utils.h"
#include <gsl/gsl>

namespace Halley {
	class Compression {
	public:
		static Bytes deflate(const Bytes& bytes);
		static Bytes deflate(gsl::span<const gsl::byte> bytes);
		static Bytes inflate(const Bytes& bytes);
		static Bytes inflate(gsl::span<const gsl::byte> bytes);
		static std::shared_ptr<const char> inflateToSharedPtr(gsl::span<const gsl::byte> bytes, size_t& outSize);
		static unsigned char* inflateRaw(gsl::span<const gsl::byte> bytes, size_t& outSize);
	};
}
