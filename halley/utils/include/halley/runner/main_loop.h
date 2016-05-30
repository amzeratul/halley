#pragma once
#include <halley/time/halleytime.h>

namespace Halley
{
	class HalleyStatics;
	class GameLoader;
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

		virtual void init() = 0;
		virtual void onSuspended() = 0;
		virtual void onReloaded() = 0;
	};

	class MainLoop
	{
	public:
		MainLoop(IMainLoopable& core, GameLoader& reloader);
		void run();

	private:
		IMainLoopable& target;
		GameLoader& reloader;

		unsigned int delay = 0;
		int fps = 60;
		bool capFrameRate = false;

		void runLoop();
		bool isRunning() const;
		bool tryReload() const;
	};
}
