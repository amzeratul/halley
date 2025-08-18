#pragma once
#include "../utils/utils.h"
#include <gsl/gsl>
#include <limits>
#include <optional>

namespace Halley {
	class Compression {
	public:
		static Bytes compress(const Bytes& bytes, int level = -1);
		static Bytes compress(gsl::span<const std::byte> bytes, int level = -1);
		static Bytes decompress(const Bytes& bytes, size_t maxSize = std::numeric_limits<size_t>::max());
		static Bytes decompress(gsl::span<const std::byte> bytes, size_t maxSize = std::numeric_limits<size_t>::max());
		static std::shared_ptr<const char> decompressToSharedPtr(gsl::span<const std::byte> bytes, size_t& outSize, size_t maxSize = std::numeric_limits<size_t>::max());
		static Bytes compressRaw(gsl::span<const std::byte> bytes, bool insertLength, int level = -1);
		static gsl::span<std::byte> compressRaw(gsl::span<const std::byte> inBytes, gsl::span<std::byte> outBytes, bool insertLength, int level = -1);
		static Bytes decompressRaw(gsl::span<const std::byte> bytes, bool headerLess, size_t maxSize, size_t expectedSize = 0);

		enum class LZ4Mode {
			Normal,
			HC
		};

		struct LZ4Options {
			LZ4Mode mode = LZ4Mode::Normal;
			int level = 9;

			LZ4Options(LZ4Mode mode = LZ4Mode::Normal, int level = 9) noexcept
				: mode(mode)
				, level(level)
			{
			}
		};

		static Bytes lz4Compress(gsl::span<const std::byte> src, LZ4Options options = {});
		static size_t lz4Compress(gsl::span<const std::byte> src, gsl::span<std::byte> dst, LZ4Options options = {});
		static size_t lz4Compress(gsl::span<const char> src, gsl::span<char> dst, LZ4Options options = {});
		static size_t lz4Compress(gsl::span<const Byte> src, gsl::span<Byte> dst, LZ4Options options = {});
		static std::optional<size_t> lz4Decompress(gsl::span<const std::byte> src, gsl::span<std::byte> dst);
		static std::optional<size_t> lz4Decompress(gsl::span<const char> src, gsl::span<char> dst);
		static std::optional<size_t> lz4Decompress(gsl::span<const Byte> src, gsl::span<Byte> dst);

		static Bytes lz4CompressFile(gsl::span<const std::byte> src, gsl::span<const std::byte> header, LZ4Options options = {});
		static Bytes lz4DecompressFile(gsl::span<const std::byte> src, gsl::span<std::byte> header);
		static std::shared_ptr<const char> lz4DecompressFileToSharedPtr(gsl::span<const std::byte> src, gsl::span<std::byte> header, size_t& outSize);
	};
}
