#include "halley/core/game/main_loop.h"
#include <halley/support/console.h>
#include <halley/support/debug.h>
#include <halley/core/game/core.h>
#include <iostream>

Halley::MainLoop::MainLoop(Core& core)
	: core(core)
{
}

void Halley::MainLoop::run(bool capFrameRate, int fps)
{
	auto& api = core.getAPI();

	std::cout << ConsoleColor(Console::GREEN) << "\nStarting main loop." << ConsoleColor() << std::endl;
	Debug::trace("MainLoop::run begin");

	using Uint32 = unsigned int;
	Uint32 startTime = 0;
	Uint32 targetTime = 0;
	Uint32 nSteps = 0;
	Uint32 lastTime = 0;

	Time fixedDelta = 1.0 / fps;

	while (core.isRunning()) {
		if (core.transitionStage()) {
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
				onFixedUpdate(fixedDelta);

				nSteps++;
				curTime = api.system->getTicks();
				targetTime = startTime + Uint32((static_cast<long long>(nSteps) * 1000) / fps);
			}
		} else {
			// Nope, release CPU
			api.system->delay(1);
		}

		onVariableUpdate((curTime - lastTime) * 0.001f);
		lastTime = curTime;
	}

	Debug::trace("MainLoop::run end");
	std::cout << ConsoleColor(Console::GREEN) << "Main loop terminated." << ConsoleColor() << std::endl;
}

void Halley::MainLoop::onVariableUpdate(Time delta)
{
	std::cout << delta << " ";
	if (core.isRunning()) {
		core.onVariableUpdate(delta);
	}

	if (core.isRunning()) {
		core.onRender(delta);
	}
}

void Halley::MainLoop::onFixedUpdate(Time delta)
{
	if (core.isRunning()) {
		core.onFixedUpdate(delta);
	}
}

bool Halley::MainLoop::isRunning() const
{
	return core.isRunning();
}
