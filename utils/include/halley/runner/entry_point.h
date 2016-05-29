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

		virtual std::unique_ptr<Game> makeGame() = 0;
		virtual void setupStatics(HalleyStatics* statics) = 0;
	};

	template <typename T>
	class HalleyEntryPoint : public IHalleyEntryPoint
	{
	public:
		std::unique_ptr<Game> makeGame() override
		{
			return std::make_unique<T>();
		}

		void setupStatics(HalleyStatics* statics) override
		{
			statics->setup();
		}
	};
}
