#pragma once

#include <boost/pool/pool.hpp>

namespace Halley {
	typedef boost::pool<boost::default_user_allocator_malloc_free> PoolType;

	// yo dawg
	class PoolPool
	{
	public:
		static PoolType* getPool(size_t size);

	private:
		static PoolPool& get();

		std::map<size_t, PoolType*> pools;
	};

	template <typename T>
	struct PoolAllocator
	{
	public:
		static void* alloc()
		{
			return get().pool->malloc();
		}

		static void free(void* p)
		{
			get().pool->free(p);
		}

	private:
		PoolAllocator()
		{
			//pool = PoolPool::getPool(sizeof(T));
			pool = new PoolType(sizeof(T));
		}

		static PoolAllocator& get()
		{
			static PoolAllocator instance;
			return instance;
		}

		boost::pool<boost::default_user_allocator_malloc_free>* pool;
	};

	/*
	template <typename T>
	struct PoolAllocator
	{
		static void* alloc()
		{
			return PoolManager<sizeof(T)>::alloc();
		}

		static void free(void* p)
		{
			PoolManager<sizeof(T)>::free(p);
		}

	private:
		PoolAllocator() {}
	};
	*/

}
