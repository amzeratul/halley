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
	capFrameRate = true;
	fps = target.getTargetFPS();

	do {
		runLoop();
	} while (tryReload());
}

void MainLoop::runLoop()
{
	std::cout << ConsoleColour(Console::GREEN) << "\nStarting main loop." << ConsoleColour() << std::endl;

	using Clock = std::chrono::steady_clock;
	using namespace std::chrono_literals;

	int64_t nSteps = 0;
	Clock::time_point startTime;
	Clock::time_point targetTime;
	Clock::time_point lastFrameStartTime;
	startTime = targetTime = lastFrameStartTime = Clock::now();

	auto frameTimes = RollingDataSet<Clock::time_point>(13);

	while (isRunning()) {
		fps = target.getTargetFPS();
		if (fps <= 0) {
			target.transitionStage();
			constexpr Time fixedDelta = 1.0 / 60.0;
			target.onFixedUpdate(fixedDelta);
			target.onTick(fixedDelta, [] (bool) {});
		} else {
			if (target.transitionStage()) {
				// Reset counters
				startTime = targetTime = lastFrameStartTime = Clock::now();
				nSteps = 0;
				frameTimes.clear();
			}
			Clock::time_point curFrameStartTime = Clock::now();

			// Got any fixed updates to do?
			if (curFrameStartTime >= targetTime) {
				// Figure out how many steps we need...
				const Time fixedDelta = 1.0 / fps;
				int stepsNeeded = int(std::chrono::duration<float>(curFrameStartTime - targetTime).count() * fps);

				// Run up to 5 (if we're more than 5 frames late, ignore them. C'est la vie)
				for (int i = 0; i < std::min(stepsNeeded, 5); i++) {
					target.onFixedUpdate(fixedDelta);
				}

				// Update target
				nSteps += stepsNeeded;
				targetTime = startTime + std::chrono::microseconds((nSteps * 1000000ll) / fps);
			} else {
				// Nope, release CPU
				std::this_thread::yield();
			}

			// Run variable update
			const Time elapsed = std::chrono::duration<double>(curFrameStartTime - lastFrameStartTime).count();
			const auto delta = std::min(elapsed, 0.1); // Never step by more than 100ms
			frameTimes.add(curFrameStartTime);
			target.onTick(delta, [&] (bool vsync)
			{
				const Time tolerance = vsync ? 0.003 : 0.0;
				const auto now = Clock::now();
				const Time wait = (1.0 * frameTimes.size()) / fps - std::chrono::duration<double>(now - frameTimes.getOldest()).count() - tolerance;

				if (wait > 0) {
					using namespace std::chrono_literals;
					std::this_thread::sleep_for(1.0s * wait);
				}
			});
			lastFrameStartTime = curFrameStartTime;
		}
	}

	std::cout << ConsoleColour(Console::GREEN) << "Main loop terminated." << ConsoleColour() << std::endl;
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
