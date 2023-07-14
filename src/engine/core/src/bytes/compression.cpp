#include <cstdlib>
#include <memory>
#include "halley/bytes/compression.h"
#include "../../../../contrib/zlib/zlib.h"
#include "halley/support/exception.h"
#include "halley/text/string_converter.h"
#include "lz4/lz4.h"
#include "lz4/lz4hc.h"

using namespace Halley;

static voidpf zlibAlloc (voidpf opaque, uInt items, uInt size)
{
	return malloc(size_t(items * size));
}

static void zlibFree(voidpf opaque, voidpf address)
{
	free(address);
}

Bytes Compression::compress(const Bytes& bytes, int level)
{
	return compress(gsl::as_bytes(gsl::span<const Byte>(bytes)), level);
}

Bytes Compression::compress(gsl::span<const gsl::byte> bytes, int level)
{
	return compressRaw(bytes, true, level);
}

Bytes Compression::decompress(const Bytes& bytes, size_t maxSize)
{
	return decompress(gsl::as_bytes(gsl::span<const Byte>(bytes)), maxSize);
}

Bytes Compression::decompress(gsl::span<const gsl::byte> bytes, size_t maxSize)
{
	Expects (sizeof(uint64_t) == 8);
	Expects (bytes.size_bytes() >= 8);
	uint64_t expectedOutSize;
	memcpy(&expectedOutSize, bytes.data(), 8);
	auto out = decompressRaw(bytes.subspan(8), maxSize, size_t(expectedOutSize));
	return out;
}

static void deleter(const char* data)
{
	delete[] data;
}

std::shared_ptr<const char> Compression::decompressToSharedPtr(gsl::span<const gsl::byte> bytes, size_t& size, size_t maxSize)
{
	auto out = decompress(bytes, maxSize);
	
	size = out.size();
	auto rawResult = new char[out.size()];
	memcpy(rawResult, out.data(), out.size());
	auto result = std::shared_ptr<const char>(rawResult, deleter);
	return result;
}

Bytes Compression::compressRaw(gsl::span<const gsl::byte> bytes, bool insertLength, int level)
{
	const uint64_t inSize = bytes.size_bytes();
	const size_t headerSize = insertLength ? 8 : 0;
	Bytes result(size_t(inSize) + headerSize + 16); // Header size, plus 16 bytes for headroom, should be enough for any compression

	auto span = compressRaw(bytes, gsl::as_writable_bytes(gsl::span<Byte>(result)), insertLength, level);
	result.resize(span.size_bytes());
	
	return result;
}

gsl::span<gsl::byte> Compression::compressRaw(gsl::span<const gsl::byte> inBytes, gsl::span<gsl::byte> outBytes, bool insertLength, int level)
{
	Expects (sizeof(uint64_t) == 8);

	const uint64_t inSize = inBytes.size_bytes();
	const size_t headerSize = insertLength ? 8 : 0;

	if (insertLength) {
		assert(outBytes.size_bytes() >= 8);
		memcpy(outBytes.data(), &inSize, 8);
	}

	z_stream stream;
	stream.zalloc = &zlibAlloc;
	stream.zfree = &zlibFree;
	stream.opaque = nullptr;
	int res = deflateInit(&stream, level);
	if (res != Z_OK) {
		throw Exception("Unable to initialize zlib compression", HalleyExceptions::Compression);
	}

	stream.avail_in = uInt(inBytes.size_bytes());
	stream.next_in = reinterpret_cast<unsigned char*>(const_cast<gsl::byte*>(inBytes.data()));
	stream.avail_out = uInt(outBytes.size() - headerSize);
	stream.next_out = reinterpret_cast<unsigned char*>(const_cast<gsl::byte*>(outBytes.data())) + headerSize;

	do {
		res = deflate(&stream, Z_FINISH);
		if (res == Z_STREAM_ERROR || res == Z_BUF_ERROR) {
			deflateEnd(&stream);
			throw Exception("Unable to compress data.", HalleyExceptions::Compression);
		}
	} while (res != Z_STREAM_END);

	const size_t outSize = size_t(stream.total_out);
	deflateEnd(&stream);

	return outBytes.subspan(0, headerSize + outSize);
}

Bytes Compression::decompressRaw(gsl::span<const gsl::byte> bytes, size_t maxSize, size_t expectedSize)
{
	if (expectedSize > uint64_t(maxSize)) {
		throw Exception("File is too big to inflate: " + String::prettySize(expectedSize), HalleyExceptions::Compression);
	}
	
	z_stream stream;
	stream.zalloc = &zlibAlloc;
	stream.zfree = &zlibFree;
	stream.opaque = nullptr;
	stream.avail_in = 0;
	stream.next_in = nullptr;
	int ret = inflateInit(&stream);
	if (ret != Z_OK) {
		throw Exception("Unable to initialise zlib", HalleyExceptions::Compression);
	}
	stream.avail_in = uInt(bytes.size_bytes());
	stream.next_in = reinterpret_cast<unsigned char*>(const_cast<gsl::byte*>(bytes.data()));

	if (expectedSize > 0) {
		Bytes result(expectedSize);
		stream.avail_out = uInt(result.size());
		stream.next_out = result.data();
		
		const int res = inflate(&stream, Z_FINISH);
		const size_t totalOut = size_t(stream.total_out);
		inflateEnd(&stream);

		if (res != Z_STREAM_END) {
			throw Exception("Unable to inflate stream.", HalleyExceptions::Compression);
		}
		if (totalOut != expectedSize) {
			throw Exception("Unexpected outsize (" + toString(result.size()) + ") when inflating data, expected (" + toString(expectedSize) + ").", HalleyExceptions::Compression);
		}

		return result;
	} else {
		constexpr size_t blockSize = 256 * 1024;
		Bytes result(std::min(alignUp(bytes.size_bytes() * 2, blockSize), maxSize));

		int res = 0;
		do {
			// Expand if needed
			if (result.size() - size_t(stream.total_out) < blockSize / 2) {
				if (result.size() >= maxSize) {
					inflateEnd(&stream);
					throw Exception("Unable to inflate stream, maximum size has been exceeded.", HalleyExceptions::Compression);
				}
				const auto newSize = std::min(result.size() * 2, maxSize);
				result.resize(newSize);
			}
			stream.avail_out = uInt(result.size()) - stream.total_out;
			stream.next_out = result.data() + size_t(stream.total_out);
			res = inflate(&stream, Z_NO_FLUSH);
		} while (res == Z_OK);

		const size_t totalOut = size_t(stream.total_out);
		inflateEnd(&stream);

		if (res != Z_STREAM_END) {
			throw Exception("Unable to inflate stream.", HalleyExceptions::Compression);
		}
		result.resize(totalOut);

		return result;
	}
}

Bytes Compression::lz4Compress(gsl::span<const gsl::byte> src, LZ4Options options)
{
	const auto size = LZ4_compressBound(static_cast<int>(src.size()));
	Bytes result;
	result.resize_no_init(size);
	const auto outSize = lz4Compress(src, result.byte_span(), options);
	result.resize(outSize);
	return result;
}

struct LZ4FileHeader {
	char id[4] = "LZ4";
	uint32_t size = 0;
};

Bytes Compression::lz4CompressFile(gsl::span<const gsl::byte> src, gsl::span<const gsl::byte> header, LZ4Options options)
{
	constexpr size_t lz4HeaderSize = sizeof(LZ4FileHeader);
	const size_t totalHeaderSize = lz4HeaderSize + header.size_bytes();

	const auto size = LZ4_compressBound(static_cast<int>(src.size()));
	Bytes result;
	result.resize_no_init(size + totalHeaderSize);
	const auto outSize = lz4Compress(src, result.byte_span().subspan(totalHeaderSize), options);
	result.resize(outSize + totalHeaderSize);

	LZ4FileHeader lz4Header;
	lz4Header.size = static_cast<uint32_t>(src.size_bytes());
	memcpy(result.data(), &lz4Header, sizeof(lz4Header));

	if (!header.empty()) {
		memcpy(result.data() + lz4HeaderSize, header.data(), header.size_bytes());
	}

	return result;
}

Bytes Compression::lz4DecompressFile(gsl::span<const gsl::byte> src, gsl::span<gsl::byte> header)
{
	constexpr size_t lz4HeaderSize = sizeof(LZ4FileHeader);
	const size_t totalHeaderSize = lz4HeaderSize + header.size_bytes();

	if (src.size() < totalHeaderSize) {
		throw Exception("File too small to be LZ4 file", HalleyExceptions::Utils);
	}

	LZ4FileHeader lz4Header;
	memcpy(&lz4Header, src.data(), sizeof(lz4Header));
	if (memcmp(lz4Header.id, "LZ4", 4) != 0) {
		throw Exception("Not LZ4 header file", HalleyExceptions::Utils);
	}
	
	if (!header.empty()) {
		memcpy(header.data(), src.data() + lz4HeaderSize, header.size_bytes());
	}

	Bytes output;
	output.resize_no_init(lz4Header.size);
	const auto outSize = lz4Decompress(src.subspan(totalHeaderSize), output.byte_span());
	output.resize(outSize.value_or(0));
	return output;
}

std::shared_ptr<const char> Compression::lz4DecompressFileToSharedPtr(gsl::span<const gsl::byte> src, gsl::span<gsl::byte> header, size_t& outSize)
{
	constexpr size_t lz4HeaderSize = sizeof(LZ4FileHeader);
	const size_t totalHeaderSize = lz4HeaderSize + header.size_bytes();

	if (src.size() < totalHeaderSize) {
		throw Exception("File too small to be LZ4 file", HalleyExceptions::Utils);
	}

	LZ4FileHeader lz4Header;
	memcpy(&lz4Header, src.data(), sizeof(lz4Header));
	if (memcmp(lz4Header.id, "LZ4", 4) != 0) {
		throw Exception("Not LZ4 header file", HalleyExceptions::Utils);
	}

	memcpy(header.data(), src.data() + lz4HeaderSize, header.size_bytes());

	auto output = std::shared_ptr<char>(new char[lz4Header.size], deleter);
	const auto sz = lz4Decompress(src.subspan(totalHeaderSize), gsl::as_writable_bytes(gsl::span<char>(output.get(), lz4Header.size)));
	if (!sz) {
		throw Exception("Failed to decompress LZ4 file", HalleyExceptions::Utils);
	}
	assert(sz == lz4Header.size);
	outSize = *sz;
	return output;
}

size_t Compression::lz4Compress(gsl::span<const gsl::byte> src, gsl::span<gsl::byte> dst, LZ4Options options)
{
	if (options.mode == LZ4Mode::Normal) {
		return LZ4_compress_default(reinterpret_cast<const char*>(src.data()), reinterpret_cast<char*>(dst.data()), static_cast<int>(src.size_bytes()), static_cast<int>(dst.size_bytes()));
	} else {
		return LZ4_compress_HC(reinterpret_cast<const char*>(src.data()), reinterpret_cast<char*>(dst.data()), static_cast<int>(src.size_bytes()), static_cast<int>(dst.size_bytes()), options.level);
	}
}

size_t Compression::lz4Compress(gsl::span<const char> src, gsl::span<char> dst, LZ4Options options)
{
	return lz4Compress(gsl::as_bytes(src), gsl::as_writable_bytes(dst), options);
}

size_t Compression::lz4Compress(gsl::span<const Byte> src, gsl::span<Byte> dst, LZ4Options options)
{
	return lz4Compress(gsl::as_bytes(src), gsl::as_writable_bytes(dst), options);
}

std::optional<size_t> Compression::lz4Decompress(gsl::span<const gsl::byte> src, gsl::span<gsl::byte> dst)
{
	const auto result = LZ4_decompress_safe(reinterpret_cast<const char*>(src.data()), reinterpret_cast<char*>(dst.data()), static_cast<int>(src.size_bytes()), static_cast<int>(dst.size_bytes()));
	if (result >= 0) {
		return result;
	} else {
		return std::nullopt;
	}
}

std::optional<size_t> Compression::lz4Decompress(gsl::span<const char> src, gsl::span<char> dst)
{
	return lz4Decompress(gsl::as_bytes(src), gsl::as_writable_bytes(dst));
}

std::optional<size_t> Compression::lz4Decompress(gsl::span<const Byte> src, gsl::span<Byte> dst)
{
	return lz4Decompress(gsl::as_bytes(src), gsl::as_writable_bytes(dst));
}
