#include "halley/utils/hash.h"
#include "../contrib/xxhash/xxhash.h"

using namespace Halley;

uint64_t Hash::hash(const Bytes& bytes)
{
	return hash(gsl::as_bytes(gsl::span<const Byte>(bytes)));
}

uint64_t Hash::hash(gsl::span<const std::byte> bytes)
{
	return XXH64(bytes.data(), size_t(bytes.size_bytes()), 0);
}

uint32_t Hash::compressTo32(uint64_t value)
{
	const auto high = value >> 32;
	const auto low = value & 0xFFFFFFFFull;
	return uint32_t(high) ^ uint32_t(low);
}

Hash::Hasher::Hasher()
{
	auto d = XXH3_createState();
	XXH3_64bits_reset(d);
	data = d;
}

Hash::Hasher::~Hasher()
{
	XXH3_freeState(static_cast<XXH3_state_t*>(data));
}

void Hash::Hasher::feedBytes(gsl::span<const std::byte> bytes)
{
	XXH3_64bits_update(static_cast<XXH3_state_t*>(data), bytes.data(), size_t(bytes.size_bytes()));
}

uint64_t Hash::Hasher::digest()
{
	return XXH3_64bits_digest(static_cast<XXH3_state_t*>(data));
}

void Hash::Hasher::reset()
{
	XXH3_64bits_reset(static_cast<XXH3_state_t*>(data));
}
