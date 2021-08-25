#pragma once

#include "flat_map.h"
#include <mutex>

namespace Halley {
	class SizePool
	{
	public:
		explicit SizePool(size_t size);
		~SizePool();

		size_t getSize() const { return size; }
		void* alloc();
		void free(void* p);

	private:
		void* pimpl;
		size_t size;
		std::mutex mutex;
	};

	// yo dawg
	class PoolPool
	{
	public:
		static SizePool* getPool(size_t size);

	private:
		static PoolPool& get();

		FlatMap<size_t, SizePool*> pools;
	};

	template <typename T>
	struct PoolAllocator
	{
	public:
		PoolAllocator()
		{
			pool = new SizePool(sizeof(T));
		}
		
		void* alloc()
		{
			return pool->alloc();
		}

		void free(void* p)
		{
			pool->free(p);
		}

	private:

		SizePool* pool;
	};
	
}
