#pragma once

namespace Halley {
	class TempMemoryPool {
	public:
		TempMemoryPool(size_t capacity);
		~TempMemoryPool();

		char* allocate(size_t n, size_t alignment);
		void deallocate(void* ptr, size_t n);

		void reset();
		void resize(size_t size);

	private:
		size_t capacity = 0;
		size_t pos = 0;
		size_t allocated = 0;
		char* data = nullptr;

		void allocBuffer();
		void freeBuffer();
	};

	template <typename T, typename Pool>
	class PoolAllocator {
	public:
		typedef T value_type;

		PoolAllocator()
			: pool(nullptr)
		{}

		PoolAllocator(Pool& pool)
			: pool(&pool)
		{}

		template<class U>
		constexpr PoolAllocator(const PoolAllocator<U, Pool>& other) noexcept
			: pool(other.pool)
		{}

		[[nodiscard]] T* allocate(std::size_t n)
		{
			assert(pool != nullptr);
			return reinterpret_cast<T*>(pool->allocate(n * sizeof(T), alignof(T)));
		}

		void deallocate(T* p, std::size_t n) noexcept
		{
			assert(pool != nullptr);
			return pool->deallocate(p, n * sizeof(T));
		}

		Pool* pool;
	};

	template <typename T>
	using TempPoolAllocator = PoolAllocator<T, TempMemoryPool>;
}
