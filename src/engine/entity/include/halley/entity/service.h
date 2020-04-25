#pragma once

#include <typeinfo>

namespace Halley
{
	class Service
	{
	public:
		Service() = default;
		Service(const Service& other) = delete;
		Service(Service&& other) noexcept = default;
		virtual ~Service() = default;

		Service& operator=(const Service& other) = delete;
		Service& operator=(Service&& other) noexcept = default;

		String getName() const { return typeid(*this).name(); }
	};
}