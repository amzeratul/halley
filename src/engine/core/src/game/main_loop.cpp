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
		runLoop();
	} while (tryReload());
}

void MainLoop::runStep()
{
	Clock::time_point curFrameStartTime = Clock::now();

	if (target.transitionStage()) {
		lastFrameStartTime = {};
	}

	const Time dt = lastFrameStartTime ? std::chrono::duration<double>(curFrameStartTime - *lastFrameStartTime).count() : 0.0;
	target.onTick(dt);

	lastFrameStartTime = curFrameStartTime;
}

void MainLoop::runLoop()
{
	std::cout << ConsoleColour(Console::GREEN) << "\nStarting main loop." << ConsoleColour() << std::endl;

	using namespace std::chrono_literals;

	Clock::time_point lastFrameStartTime = Clock::now();
	auto frameTimes = RollingDataSet<Time>(30, false);
	double lastSpareTime = 0;
	double leftOverTime = 0;
	double previousTargetFPS = 0;

	while (isRunning()) {
		if (target.transitionStage()) {
			frameTimes.clear();
			lastSpareTime = 0;
		}

		if (target.getTargetFPS() != previousTargetFPS) {
			previousTargetFPS = target.getTargetFPS();
			frameTimes.clear();
		}

		Clock::time_point curFrameStartTime = Clock::now();
		const auto measuredElapsed = std::chrono::duration<double>(curFrameStartTime - lastFrameStartTime).count();
		frameTimes.add(measuredElapsed);

		// Compute the real elapsed time and the tick length
		// The latter will be fudged to account for timing jitter, as it leads to smoother gameplay
		const double fps = target.getTargetFPS();
		const std::optional<Time> elapsedTarget = fps > 0 ? 1.0 / fps : std::optional<Time>();
		const Time tickLength = clamp(snapElapsedTime(measuredElapsed + leftOverTime, elapsedTarget, frameTimes), 0.000001, 0.100);
		leftOverTime = measuredElapsed - tickLength;

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

Time MainLoop::snapElapsedTime(Time measuredElapsed, std::optional<Time> desired, const RollingDataSet<Time>& frameTimes)
{
	Time elapsed = measuredElapsed;

	const auto avgFrameLen = frameTimes.size() > 1 ? frameTimes.getMedian() : measuredElapsed;
	if (desired) {
		if (std::abs(avgFrameLen - *desired) <= 0.001) {
			// Snap frame time if average has been within 1ms
			elapsed = *desired;
		}
	} else {
		const auto avgFps = 1.0f / avgFrameLen;
		const auto knownFPS = std::to_array({ 30.0f, 40.0f, 50.0f, 60.0f, 75.0f, 85.0f, 90.0f, 100.0f, 120.0f, 144.0f, 165.0f, 180.0f, 240.0f, 300.0f, 360.0f, 500.0f });
		for (const auto fps: knownFPS) {
			if (std::abs(avgFps - fps) <= 2) {
				elapsed = 1.0 / fps;
				break;
			}
		}
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
