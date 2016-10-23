#pragma once

#include <typeinfo>

namespace Halley
{
	class Service
	{
	public:
		virtual ~Service() {}
		String getName() const { return typeid(*this).name(); }
	};
}