#pragma once

#include <new>
#include <cstddef>

namespace Halley
{
	class Component
	{
	public:
		void* operator new(size_t size);
		void operator delete(void* ptr) noexcept(false);
	};
}

