#include <cstdlib>
#include <memory>
#include "halley/file/compression.h"
#include "../../contrib/lodepng/lodepng.h"
#include "halley/support/exception.h"
#include "halley/text/string_converter.h"

using namespace Halley;

Bytes Compression::deflate(const Bytes& bytes)
{
	return deflate(gsl::as_bytes(gsl::span<const Byte>(bytes)));
}

Bytes Compression::deflate(gsl::span<const gsl::byte> bytes)
{
	Expects (sizeof(uint64_t) == 8);

	uint64_t inSize = bytes.size_bytes();
	size_t outSize = 0;
	unsigned char* out = nullptr;
	
	LodePNGCompressSettings settings;
	lodepng_compress_settings_init(&settings);
	lodepng_zlib_compress(&out, &outSize, reinterpret_cast<const unsigned char*>(bytes.data()), inSize, &settings);

	Bytes result(outSize + 8);
	memcpy(result.data(), &inSize, 8);
	memcpy(result.data() + 8, out, outSize);
	free(out);
	
	return result;
}

Bytes Compression::inflate(const Bytes& bytes, size_t maxSize)
{
	return inflate(gsl::as_bytes(gsl::span<const Byte>(bytes)), maxSize);
}

Bytes Compression::inflate(gsl::span<const gsl::byte> bytes, size_t maxSize)
{
	size_t outSize;
	auto b = inflateToSharedPtr(bytes, outSize, maxSize);
	Bytes result(outSize);
	memcpy(result.data(), b.get(), outSize);
	return result;
}

static void deleter(const char* data)
{
	delete[] data;
}

std::shared_ptr<const char> Compression::inflateToSharedPtr(gsl::span<const gsl::byte> bytes, size_t& size, size_t maxSize)
{
	Expects (sizeof(uint64_t) == 8);
	Expects (bytes.size_bytes() >= 8);
	uint64_t expectedOutSize;
	memcpy(&expectedOutSize, bytes.data(), 8);
	if (expectedOutSize > uint64_t(maxSize)) {
		throw Exception("File is too big to inflate: " + String::prettySize(expectedOutSize), HalleyExceptions::File);
	}

	size_t outSize = 0;
	auto out = inflateRaw(bytes.subspan(8), outSize, maxSize);

	if (outSize != expectedOutSize) {
		throw Exception("Unexpected outsize (" + toString(outSize) + ") when inflating data, expected (" + toString(expectedOutSize) + ").", HalleyExceptions::File);
	}
	
	size = outSize;
	auto rawResult = new char[outSize];
	memcpy(rawResult, out, outSize);
	auto result = std::shared_ptr<const char>(rawResult, deleter);
	free(out);
	return result;
}

unsigned char* Compression::inflateRaw(gsl::span<const gsl::byte> bytes, size_t& outSize, size_t maxSize)
{
	unsigned char* out = nullptr;
	LodePNGDecompressSettings settings;
	lodepng_decompress_settings_init(&settings);
	lodepng_zlib_decompress(&out, &outSize, reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size_bytes(), &settings);
	return out;
}
