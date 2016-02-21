#include "memory_pool.h"

using namespace Halley;

PoolPool& PoolPool::get()
{
	static PoolPool* pools = nullptr;
	if (!pools) {
		pools = new PoolPool();
	}
	return *pools;
}

PoolType* PoolPool::getPool(size_t size)
{
	auto& pools = get().pools;
	auto iter = pools.find(size);
	if (iter != pools.end()) {
		return iter->second;
	}

	auto pool = new PoolType(size);
	pools[size] = pool;
	return pool;
}
