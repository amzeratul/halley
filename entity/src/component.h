#pragma once

#include <new>

namespace Halley
{
	class Component
	{
	public:
		void* operator new(size_t size);
		void operator delete(void* ptr) noexcept(false);
	};
}

