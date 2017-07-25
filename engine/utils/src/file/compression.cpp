#include <cstdlib>
#include <memory>
#include "halley/file/compression.h"
#include "../../contrib/lodepng/lodepng.h"

using namespace Halley;

Bytes Compression::deflate(gsl::span<const gsl::byte> bytes)
{
	Expects (sizeof(uint64_t) == 8);

	size_t outSize = 0;
	unsigned char* out = nullptr;
	
	LodePNGCompressSettings settings;
	lodepng_compress_settings_init(&settings);
	lodepng_zlib_compress(&out, &outSize, reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size_bytes(), &settings);

	Bytes result(outSize + 8);
	*reinterpret_cast<uint64_t*>(result.data()) = outSize;
	memcpy(result.data() + 8, out, outSize);
	free(out);
	
	return result;
}

Bytes Compression::inflate(gsl::span<const gsl::byte> bytes)
{
	size_t outSize;
	auto b = inflateRaw(bytes, outSize);
	Bytes result(outSize);
	memcpy(result.data(), b.get(), outSize);
	return result;
}

static void deleter(const char* data)
{
	delete[] data;
}

std::shared_ptr<const char> Compression::inflateRaw(gsl::span<const gsl::byte> bytes, size_t& size)
{
	Expects (sizeof(uint64_t) == 8);
	Expects (bytes.size_bytes() >= 8);
	uint64_t expectedOutSize = *reinterpret_cast<const uint64_t*>(bytes.data());
	Expects (bytes.size_bytes() == expectedOutSize + 8);

	unsigned char* out = nullptr;
	size_t outSize = 0;
	LodePNGDecompressSettings settings;
	lodepng_decompress_settings_init(&settings);
	lodepng_zlib_decompress(&out, &outSize, reinterpret_cast<const unsigned char*>(bytes.data() + 8), bytes.size_bytes() - 8, &settings);
	
	Expects(outSize == expectedOutSize);
	
	size = outSize;
	auto rawResult = new char[outSize];
	memcpy(rawResult, out, outSize);
	auto result = std::shared_ptr<const char>(rawResult, deleter);
	free(out);
	return result;
}
