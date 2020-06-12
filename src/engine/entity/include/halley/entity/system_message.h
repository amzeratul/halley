#pragma once

#include <new>
#include <cstddef>

namespace Halley
{
	class SystemMessage
	{
	public:
		virtual ~SystemMessage() {}
		virtual size_t getSize() const = 0;

		/*
		void* operator new(size_t size);
		void operator delete(void* ptr);
		*/
	};
}
