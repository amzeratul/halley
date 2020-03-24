#pragma once

#include <memory>
#include <halley/core/game/game.h>
#include "entry_point.h"
#include "halley/core/game/core.h"

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

		virtual std::unique_ptr<Core> createCore(Vector<std::string> args)
		{
			return std::make_unique<Core>(createGame(), args);
		}
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

	class EntryPointGameLoader : public GameLoader
	{
	public:
		EntryPointGameLoader(IHalleyEntryPoint& entryPoint)
			: entryPoint(entryPoint)
		{			
		}
		
		std::unique_ptr<Game> createGame() override
		{
			return entryPoint.createGame();
		}

	private:
		IHalleyEntryPoint& entryPoint;
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
