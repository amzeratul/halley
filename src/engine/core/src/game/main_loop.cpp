#include "halley/game/main_loop.h"
#include "halley/entry/game_loader.h"

#include <iostream>

#include <halley/support/console.h>
#include <halley/support/debug.h>

#include "halley/api/halley_api.h"

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
		runStep();
	} while (tryReload());
}

void MainLoop::runStep()
{
	std::cout << ConsoleColour(Console::GREEN) << "\nStarting main loop." << ConsoleColour() << std::endl;

	using namespace std::chrono_literals;

	Clock::time_point lastFrameStartTime = Clock::now();
	auto frameTimes = RollingDataSet<Clock::time_point>(13);
	double lastSpareTime = 0;

	while (isRunning()) {
		if (target.transitionStage()) {
			frameTimes.clear();
			lastSpareTime = 0;
		}

		Clock::time_point curFrameStartTime = Clock::now();
		frameTimes.add(curFrameStartTime);

		// Compute the real elapsed time and the tick length
		// The latter will be fudged to account for timing jitter, as it leads to smoother gameplay
		const double fps = target.getTargetFPS();
		const std::optional<Time> elapsedTarget = fps > 0 ? 1.0 / fps : std::optional<Time>();
		const Time measuredElapsed = std::chrono::duration<double>(curFrameStartTime - lastFrameStartTime).count();
		const Time tickLength = std::min(snapElapsedTime(measuredElapsed, elapsedTarget, frameTimes), 0.100);

		target.onTick(tickLength);

		if (fps > 0) {
			// Framerate targetting is enabled, so we need to get rid of any excess time
			while (true) {
				const auto now = Clock::now();
				const auto totalFrameTime = std::chrono::duration<double>(now - curFrameStartTime).count();
				const auto spareTime = elapsedTarget.value_or(0) + lastSpareTime - totalFrameTime;

				if (spareTime > 0.002) {
					// We have too much spare time
					if (target.hasVsync()) {
						// Wait for another vsync
						target.waitForVsync();
					} else {
						// Wait the remaining time
						std::this_thread::sleep_for(1.0s * spareTime);
					}
				} else {
					// Accumulate spare time to adjust next frame
					lastSpareTime = spareTime;
					break;
				}
			}
		}

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

	//Logger::logDev(toString(elapsed));
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
