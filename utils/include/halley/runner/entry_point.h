#pragma once

#include <memory>
#include <vector>

namespace Halley
{
	class Game;
	
	class IHalleyEntryPoint
	{
	public:
		virtual ~IHalleyEntryPoint() {}

		virtual std::unique_ptr<IMainLoopable> createCore(std::vector<std::string> args) = 0;
	};

	template <typename T>
	class HalleyEntryPoint : public IHalleyEntryPoint
	{
	public:
		std::unique_ptr<IMainLoopable> createCore(std::vector<std::string> args) override
		{
			return std::make_unique<Core>(std::make_unique<T>(), args);
		}
	};
}
