#pragma once

#include <new>
#include <cstddef>
#include <typeinfo>

#include "halley/support/exception.h"

namespace Halley
{
	class Deserializer;
	class Serializer;

	class Message
	{
	public:
		virtual ~Message() {}
		virtual size_t getSize() const = 0;

		virtual void serialize(Serializer& s) const
		{
			throw Exception("Message " + String(typeid(*this).name()) + " is not serializable.", HalleyExceptions::Entity);
		}
		
		virtual void deserialize(Deserializer& s)
		{
			throw Exception("Message " + String(typeid(*this).name()) + " is not serializable.", HalleyExceptions::Entity);
		}
		
		/*
		void* operator new(size_t size);
		void operator delete(void* ptr);
		*/
	};
}
