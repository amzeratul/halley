#include <boost/pool/pool.hpp>
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

typedef boost::pool<boost::default_user_allocator_malloc_free> PoolType;

SizePool::SizePool(size_t size)
	: size(size)
{
	pimpl = new PoolType(size);
}

SizePool::~SizePool()
{
	delete reinterpret_cast<PoolType*>(pimpl);
}

void* SizePool::alloc()
{
	return reinterpret_cast<PoolType*>(pimpl)->malloc();
}

void SizePool::free(void* p)
{
	reinterpret_cast<PoolType*>(pimpl)->free(p);
}
