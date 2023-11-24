#pragma once
#include <halley/time/halleytime.h>
#include <string>
#include <chrono>
#include <optional>

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
		virtual void onTick(Time delta) = 0;

		virtual void init() = 0;
		virtual void onSuspended() = 0;
		virtual void onReloaded() = 0;
		virtual void onTerminatedInError(const std::string& error) = 0;

		virtual double getTargetFPS() = 0;
		virtual bool hasVsync() = 0;
		virtual void waitForVsync() = 0;
	};

	class MainLoop
	{
	public:
		MainLoop(IMainLoopable& core, GameLoader& reloader);
		void run();
		void runStep();

	private:
		IMainLoopable& target;
		GameLoader& reloader;

		using Clock = std::chrono::high_resolution_clock;
		std::optional<Clock::time_point> lastFrameStartTime;

		Time snapElapsedTime(Time measuredElapsed, std::optional<Time> desired, RollingDataSet<Clock::time_point>& frameTimes);
		bool isRunning() const;
		bool tryReload() const;

		void runLoop();
	};
}
