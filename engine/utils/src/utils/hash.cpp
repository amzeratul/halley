#include "halley/utils/hash.h"
#include "../contrib/xxhash/xxhash.h"

using namespace Halley;

uint64_t Hash::hash(const Bytes& bytes)
{
	return hash(gsl::as_bytes(gsl::span<const Byte>(bytes)));
}

uint64_t Hash::hash(gsl::span<const gsl::byte> bytes)
{
	return XXH64(bytes.data(), size_t(bytes.size_bytes()), 0);
}

uint32_t Hash::compressTo32(uint64_t value)
{
	const auto high = value >> 32;
	const auto low = value & 0xFFFFFFFFull;
	return uint32_t(high) ^ uint32_t(low);
}
