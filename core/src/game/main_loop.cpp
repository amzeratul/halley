#include "halley/core/game/main_loop.h"
#include <iostream>
#include <halley/support/console.h>
#include <halley/support/debug.h>
#include <halley/time/halleytime.h>
#include <halley/core/game/core.h>

void Halley::MainLoop::run(Core& core, bool capFrameRate, int fps)
{
	core.transitionStage();
	auto& api = core.getAPI();

	std::cout << ConsoleColor(Console::GREEN) << "\nStarting main loop." << ConsoleColor() << std::endl;
	Debug::trace("Game::runMainLoop begin");

	using Uint32 = unsigned int;

	// Set up the counters
	if (api.video) {
		api.video->flip();
	}
	Uint32 startTime = api.system->getTicks();
	Uint32 targetTime = startTime;
	Uint32 nSteps = 0;

	while (core.isRunning()) {
		Time delta = 1.0 / fps;

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
				// Fixed step
				if (core.isRunning()) {
					core.onFixedUpdate(delta);
				}

				nSteps++;
				curTime = api.system->getTicks();
				targetTime = startTime + Uint32(((long long)nSteps * 1000) / fps);
			}
		}
		else {
			// Nope, release CPU
			api.system->delay(1);
		}

		// Variable step
		if (core.isRunning()) {
			core.onVariableUpdate(delta);
		}

		// Render screen
		if (core.isRunning()) {
			core.onRender(delta);
		}

		// Switch stage
		core.transitionStage();
	}

	Debug::trace("Game::runMainLoop end");
	std::cout << ConsoleColor(Console::GREEN) << "Main loop terminated." << ConsoleColor() << std::endl;
}
