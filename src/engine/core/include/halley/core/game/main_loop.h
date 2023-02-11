#pragma once
#include <halley/time/halleytime.h>
#include <string>
#include <chrono>

#include "halley/maths/rolling_data_set.h"

namespace Halley
{
	class HalleyStatics;
	class GameLoader;
	class HalleyAPI;

	class IMainLoopable
	{
		using Clock = std::chrono::steady_clock;
	public:
		virtual ~IMainLoopable() {}

		virtual const HalleyAPI& getAPI() const = 0;
		virtual bool transitionStage() = 0;
		virtual bool isRunning() const = 0;
		virtual void onTick(Time delta, std::function<void(bool)> preVsyncWait) = 0;

		virtual void init() = 0;
		virtual void onSuspended() = 0;
		virtual void onReloaded() = 0;
		virtual void onTerminatedInError(const std::string& error) = 0;

		virtual double getTargetFPS() = 0;
	};

	class MainLoop
	{
	public:
		MainLoop(IMainLoopable& core, GameLoader& reloader);
		void run();

	private:
		IMainLoopable& target;
		GameLoader& reloader;

		using Clock = std::chrono::high_resolution_clock;

		void runLoop();
		Time snapElapsedTime(Time measuredElapsed, std::optional<Time> desired, RollingDataSet<Clock::time_point>& frameTimes);
		bool isRunning() const;
		bool tryReload() const;
	};
}
