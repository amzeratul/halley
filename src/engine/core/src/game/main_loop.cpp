#include "game/main_loop.h"
#include "entry/game_loader.h"

#include <iostream>

#include <halley/support/console.h>
#include <halley/support/debug.h>

#include "halley/core/api/halley_api.h"

#include <chrono>
#include <thread>
#include <cstdint>

#include "halley/maths/rolling_data_set.h"

using namespace Halley;

MainLoop::MainLoop(IMainLoopable& target, GameLoader& reloader)
	: target(target)
	, reloader(reloader)
{
}

void MainLoop::run()
{
	do {
		runLoop();
	} while (tryReload());
}

void MainLoop::runLoop()
{
	std::cout << ConsoleColour(Console::GREEN) << "\nStarting main loop." << ConsoleColour() << std::endl;

	using namespace std::chrono_literals;

	Clock::time_point lastFrameStartTime = Clock::now();
	auto frameTimes = RollingDataSet<Clock::time_point>(13);

	while (isRunning()) {
		if (target.transitionStage()) {
			frameTimes.clear();
		}

		Clock::time_point curFrameStartTime = Clock::now();
		frameTimes.add(curFrameStartTime);

		const double fps = target.getTargetFPS();
		const std::optional<Time> elapsedTarget = fps > 0 ? 1.0 / fps : std::optional<Time>();
		const Time measuredElapsed = std::chrono::duration<double>(curFrameStartTime - lastFrameStartTime).count();
		Time elapsed = snapElapsedTime(measuredElapsed, elapsedTarget, frameTimes);

		const auto delta = std::min(elapsed, 0.100); // Never step by more than 100ms
		target.onTick(delta, [&] (bool vsync)
		{
			if (fps > 0) {
				const Time tolerance = vsync ? 0.003 : 0.0; // 3 ms tolerance if vsync is on. This is to make sure we don't overshoot and miss vsync
				const auto now = Clock::now();
				const Time wait = (1.0 * frameTimes.size()) / fps - std::chrono::duration<double>(now - frameTimes.getOldest()).count() - tolerance;

				if (wait > 0) {
					using namespace std::chrono_literals;
					std::this_thread::sleep_for(1.0s * wait);
				}
			}
		});

		lastFrameStartTime = curFrameStartTime;
	}

	std::cout << ConsoleColour(Console::GREEN) << "Main loop terminated." << ConsoleColour() << std::endl;
}

Time MainLoop::snapElapsedTime(Time measuredElapsed, std::optional<Time> desired, RollingDataSet<Clock::time_point>& frameTimes)
{
	Time elapsed = measuredElapsed;

	const auto frameTimesTotal = std::chrono::duration<double>(frameTimes.getLatest() - frameTimes.getOldest()).count();
	const auto avgFrameLen = frameTimesTotal / (frameTimes.size() - 1);
	if (desired) {
		if (std::abs(avgFrameLen - *desired) <= 0.001) {
			// Snap frame time if average has been within 1ms
			elapsed = *desired;
		}
	} else if (std::abs(avgFrameLen - 1.0 / 60.0) <= 0.001) {
		elapsed = 1.0 / 60.0;
	} else if (std::abs(avgFrameLen - 1.0 / 120.0) <= 0.001) {
		elapsed = 1.0 / 120.0;
	} else if (std::abs(avgFrameLen - 1.0 / 144.0) <= 0.001) {
		elapsed = 1.0 / 144.0;
	}

	Logger::logDev(toString(elapsed));
	return elapsed;
}

bool MainLoop::isRunning() const
{
	return target.isRunning() && !reloader.needsToReload();
}

bool MainLoop::tryReload() const
{
	if (reloader.needsToReload()) {
		reloader.reload();
		return true;
	}
	return false;
}
