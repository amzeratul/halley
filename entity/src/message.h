#pragma once

#include <new>

namespace Halley
{
	class Message
	{
	public:
		virtual ~Message() {}
		virtual size_t getSize() const = 0;

		/*
		void* operator new(size_t size);
		void operator delete(void* ptr);
		*/
	};
}
