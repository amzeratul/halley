#pragma once

#include <new>
#include <cstddef>
#include "halley/data_structures/simple_pool.h"

namespace Halley
{
	class Component
	{
	protected:
		template <typename T>
		static void* doNew(size_t size, std::align_val_t alignment)
		{
			assert(size <= sizeof(T) && static_cast<size_t>(alignment) <= alignof(T));
			return getPool<T>().alloc();
		}

		template <typename T>
		static void* doNew(size_t size)
		{
			assert(size <= sizeof(T));
			return getPool<T>().alloc();
		}

		template <typename T>
		static void doDelete(void* ptr)
		{
			return getPool<T>().free(static_cast<T*>(ptr));
		}

		template <typename T>
		static TypedPool<T, 4096>& getPool()
		{
			static TypedPool<T, 4096> pool;
			return pool;
		}

		//void* operator new(size_t size);
		//void operator delete(void* ptr);
	};
}

