#include <iostream>

#include <halley/support/console.h>
#include <halley/support/debug.h>
#include <halley/runner/main_loop.h>
#include <halley/runner/game_loader.h>

#include "halley/core/api/halley_api.h"

#include <chrono>
#include <thread>

Halley::MainLoop::MainLoop(IMainLoopable& target, GameLoader& reloader)
	: target(target)
	, reloader(reloader)
{
}

void Halley::MainLoop::run()
{
	capFrameRate = true;
	fps = 60;

	do {
		runLoop();
	} while (tryReload());
}

void Halley::MainLoop::runLoop()
{
	std::cout << ConsoleColour(Console::GREEN) << "\nStarting main loop." << ConsoleColour() << std::endl;
	Debug::trace("MainLoop::run begin");

	using Clock = std::chrono::steady_clock;
	using namespace std::chrono_literals;

	int nSteps = 0;
	Clock::time_point startTime;
	Clock::time_point targetTime;
	Clock::time_point lastTime;
	startTime = targetTime = lastTime = Clock::now();

	Time fixedDelta = 1.0 / fps;

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
				target.onFixedUpdate(fixedDelta);

				nSteps++;
				curTime = Clock::now();
				targetTime = startTime + std::chrono::microseconds((static_cast<long long>(nSteps) * 1000000) / fps);
			}
		} else {
			// Nope, release CPU
			std::this_thread::sleep_for(1ms);
		}

		target.onVariableUpdate(std::chrono::duration<float>(curTime - lastTime).count());
		lastTime = curTime;
	}

	Debug::trace("MainLoop::run end");
	std::cout << ConsoleColour(Console::GREEN) << "Main loop terminated." << ConsoleColour() << std::endl;
}

bool Halley::MainLoop::isRunning() const
{
	return target.isRunning() && !reloader.needsToReload();
}

bool Halley::MainLoop::tryReload() const
{
	if (reloader.needsToReload()) {
		reloader.reload();
		return true;
	}
	return false;
}
