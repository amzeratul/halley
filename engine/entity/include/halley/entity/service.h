#pragma once

namespace Halley
{
	class Service
	{
	public:
		virtual ~Service() {}
		virtual String getName() const = 0;
	};
}