#pragma once
#include <halley/time/halleytime.h>

namespace Halley
{
	class HalleyAPI;

	class IMainLoopable
	{
	public:
		virtual ~IMainLoopable() {}

		virtual HalleyAPI& getAPI() const = 0;
		virtual bool transitionStage() = 0;
		virtual bool isRunning() const = 0;
		virtual void onVariableUpdate(Time delta) = 0;
		virtual void onFixedUpdate(Time delta) = 0;
		virtual void onReloaded() = 0;
	};

	class GameReloader
	{
	public:
		virtual ~GameReloader() {}

		virtual bool needsToReload() const { return false; }
		virtual void reload() {}
	};

	class MainLoop
	{
	public:
		MainLoop(IMainLoopable& core, GameReloader& reloader);
		void run();

	private:
		IMainLoopable& target;
		GameReloader& reloader;

		unsigned int delay = 0;
		int fps;
		bool capFrameRate;

		void runLoop();
		bool isRunning() const;
		bool tryReload() const;
	};
}
