#pragma once

#include <memory>
#include <vector>
#include "halley/core/game/main_loop.h"

namespace Halley
{
	class Game;
	class Core;
	
	class IHalleyEntryPoint
	{
	public:
		virtual ~IHalleyEntryPoint() {}
		virtual std::unique_ptr<Core> createCore(const std::vector<std::string>& args) = 0;
		virtual std::unique_ptr<Game> createGame() = 0;
	};

	template <typename T>
	class HalleyEntryPoint : public IHalleyEntryPoint
	{
	public:
		std::unique_ptr<Game> createGame() override
		{
			return std::make_unique<T>();
		}

		std::unique_ptr<Core> createCore(const std::vector<std::string>& args) override
		{
			Expects(args.size() >= 1);
			Expects(args.size() < 1000);
			return std::make_unique<Core>(std::make_unique<T>(), args);
		}
	};
}
