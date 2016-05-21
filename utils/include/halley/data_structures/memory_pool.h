#pragma once

#include <map>

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
	};

	// yo dawg
	class PoolPool
	{
	public:
		static SizePool* getPool(size_t size);

	private:
		static PoolPool& get();

		std::map<size_t, SizePool*> pools;
	};

	template <typename T>
	struct PoolAllocator
	{
	public:
		static void* alloc()
		{
			return get().pool->alloc();
		}

		static void free(void* p)
		{
			get().pool->free(p);
		}

	private:
		PoolAllocator()
		{
			pool = new SizePool(sizeof(T));
		}

		static PoolAllocator& get()
		{
			static PoolAllocator instance;
			return instance;
		}

		SizePool* pool;
	};
	
}
