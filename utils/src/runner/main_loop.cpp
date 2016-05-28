#include <iostream>

#include <halley/support/console.h>
#include <halley/support/debug.h>
#include <halley/runner/main_loop.h>
#include <halley/runner/game_loader.h>

#include "halley/core/api/halley_api.h"

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
	auto& api = target.getAPI();

	std::cout << ConsoleColor(Console::GREEN) << "\nStarting main loop." << ConsoleColor() << std::endl;
	Debug::trace("MainLoop::run begin");

	using Uint32 = unsigned int;
	Uint32 startTime = 0;
	Uint32 targetTime = 0;
	Uint32 nSteps = 0;
	Uint32 lastTime = 0;

	Time fixedDelta = 1.0 / fps;

	while (isRunning()) {
		if (target.transitionStage()) {
			// Reset counters
			startTime = targetTime = lastTime = api.system->getTicks();
			nSteps = 0;
		}

		if (delay > 0) {
			startTime += delay;
			targetTime += delay;
			delay = 0;
		}
		Uint32 curTime = api.system->getTicks();

		// Got anything to do?
		if (curTime >= targetTime) {
			// Step until we're up-to-date
			for (int i = 0; i < 10 && curTime >= targetTime; i++) {
				target.onFixedUpdate(fixedDelta);

				nSteps++;
				curTime = api.system->getTicks();
				targetTime = startTime + Uint32((static_cast<long long>(nSteps) * 1000) / fps);
			}
		} else {
			// Nope, release CPU
			api.system->delay(1);
		}

		target.onVariableUpdate((curTime - lastTime) * 0.001f);
		lastTime = curTime;
	}

	Debug::trace("MainLoop::run end");
	std::cout << ConsoleColor(Console::GREEN) << "Main loop terminated." << ConsoleColor() << std::endl;
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
