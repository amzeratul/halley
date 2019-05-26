#pragma once

#include <memory>
#include <halley/core/game/game.h>

namespace Halley {
	class IMainLoopable;

	class GameLoader
	{
	public:
		virtual ~GameLoader() {}

		virtual std::unique_ptr<Game> createGame() = 0;
		virtual bool needsToReload() const { return false; }
		virtual void reload() {}
		virtual void setCore(IMainLoopable&) {}
	};

	template <typename T>
	class StaticGameLoader : public GameLoader
	{
	public:
		std::unique_ptr<Game> createGame() override
		{
			return std::make_unique<T>();
		}
	};

	class DummyGameLoader : public GameLoader
	{
	public:
		std::unique_ptr<Game> createGame() override
		{
			return {};
		}
	};
}
