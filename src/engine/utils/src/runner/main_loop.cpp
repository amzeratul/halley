#include <iostream>

#include <halley/support/console.h>
#include <halley/support/debug.h>
#include <halley/runner/main_loop.h>
#include <halley/runner/game_loader.h>

#include "halley/core/api/halley_api.h"

#include <chrono>
#include <thread>
#include <cstdint>

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
	Clock::time_point lastTime;
	startTime = targetTime = lastTime = Clock::now();

	if (fps <= 0) {
		while (isRunning()) {
			target.transitionStage();
			constexpr Time fixedDelta = 1.0 / 60.0;
			target.onFixedUpdate(fixedDelta);
			target.onVariableUpdate(fixedDelta);
		}
	} else {
		while (isRunning()) {
			if (target.transitionStage()) {
				// Reset counters
				startTime = targetTime = lastTime = Clock::now();
				nSteps = 0;
			}
			Clock::time_point curTime = Clock::now();

			// Got anything to do?
			if (curTime >= targetTime) {
				// Step until we're up-to-date
				for (int i = 0; i < 10 && curTime >= targetTime; i++) {
					Time fixedDelta = 1.0 / fps;
					target.onFixedUpdate(fixedDelta);

					nSteps++;
					curTime = Clock::now();
					targetTime = startTime + std::chrono::microseconds((nSteps * 1000000ll) / fps);
				}
			} else {
				// Nope, release CPU
				std::this_thread::yield();
			}

			target.onVariableUpdate(std::chrono::duration<float>(curTime - lastTime).count());
			lastTime = curTime;
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
