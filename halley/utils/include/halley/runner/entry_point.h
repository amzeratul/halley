#pragma once

#include <memory>
#include <halley/data_structures/vector.h>

namespace Halley
{
	class Game;
	
	class IHalleyEntryPoint
	{
	public:
		virtual ~IHalleyEntryPoint() {}

		virtual std::unique_ptr<IMainLoopable> createCore(Vector<std::string> args) = 0;
	};

	template <typename T>
	class HalleyEntryPoint : public IHalleyEntryPoint
	{
	public:
		std::unique_ptr<IMainLoopable> createCore(Vector<std::string> args) override
		{
			return std::make_unique<Core>(std::make_unique<T>(), args);
		}
	};
}
