#pragma once
#include "../utils/utils.h"
#include <gsl/gsl>
#include <limits>
#include <optional>

namespace Halley {
	class Compression {
	public:
		static Bytes compress(const Bytes& bytes, int level = -1);
		static Bytes compress(gsl::span<const gsl::byte> bytes, int level = -1);
		static Bytes decompress(const Bytes& bytes, size_t maxSize = std::numeric_limits<size_t>::max());
		static Bytes decompress(gsl::span<const gsl::byte> bytes, size_t maxSize = std::numeric_limits<size_t>::max());
		static std::shared_ptr<const char> decompressToSharedPtr(gsl::span<const gsl::byte> bytes, size_t& outSize, size_t maxSize = std::numeric_limits<size_t>::max());
		static Bytes compressRaw(gsl::span<const gsl::byte> bytes, bool insertLength, int level = -1);
		static gsl::span<gsl::byte> compressRaw(gsl::span<const gsl::byte> inBytes, gsl::span<gsl::byte> outBytes, bool insertLength, int level = -1);
		static Bytes decompressRaw(gsl::span<const gsl::byte> bytes, bool headerLess, size_t maxSize, size_t expectedSize = 0);

		enum class LZ4Mode {
			Normal,
			HC
		};

		struct LZ4Options {
			LZ4Mode mode = LZ4Mode::Normal;
			int level = 9;
#if defined(__clang__) || defined(__GNUC__)
            // work around "error: default member initializer for 'mode' needed within definition
            // of enclosing class 'Compression' outside of member functions"
            LZ4Options() noexcept {}
#endif
		};

		static Bytes lz4Compress(gsl::span<const gsl::byte> src, LZ4Options options = {});
		static size_t lz4Compress(gsl::span<const gsl::byte> src, gsl::span<gsl::byte> dst, LZ4Options options = {});
		static size_t lz4Compress(gsl::span<const char> src, gsl::span<char> dst, LZ4Options options = {});
		static size_t lz4Compress(gsl::span<const Byte> src, gsl::span<Byte> dst, LZ4Options options = {});
		static std::optional<size_t> lz4Decompress(gsl::span<const gsl::byte> src, gsl::span<gsl::byte> dst);
		static std::optional<size_t> lz4Decompress(gsl::span<const char> src, gsl::span<char> dst);
		static std::optional<size_t> lz4Decompress(gsl::span<const Byte> src, gsl::span<Byte> dst);

		static Bytes lz4CompressFile(gsl::span<const gsl::byte> src, gsl::span<const gsl::byte> header, LZ4Options options = {});
		static Bytes lz4DecompressFile(gsl::span<const gsl::byte> src, gsl::span<gsl::byte> header);
		static std::shared_ptr<const char> lz4DecompressFileToSharedPtr(gsl::span<const gsl::byte> src, gsl::span<gsl::byte> header, size_t& outSize);
	};
}
