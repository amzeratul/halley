#include "halley/data_structures/memory_pool.h"

using namespace Halley;

PoolPool& PoolPool::get()
{
	static PoolPool* pools = nullptr;
	if (!pools) {
		pools = new PoolPool();
	}
	return *pools;
}

SizePool* PoolPool::getPool(size_t size)
{
	auto& pools = get().pools;
	auto iter = pools.find(size);
	if (iter != pools.end()) {
		return iter->second;
	}

	auto pool = new SizePool(size);
	pools[size] = pool;
	return pool;
}

SizePool::SizePool(size_t size)
	: size(size)
{
	throw Exception("SizePool not implemented", HalleyExceptions::Utils);
}

SizePool::~SizePool()
{
}

void* SizePool::alloc()
{
	throw Exception("SizePool not implemented", HalleyExceptions::Utils);
}

void SizePool::free(void* p)
{
	throw Exception("SizePool not implemented", HalleyExceptions::Utils);
}
