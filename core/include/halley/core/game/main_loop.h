#pragma once
#include <halley/time/halleytime.h>

namespace Halley
{
	class Core;

	class MainLoop
	{
	public:
		MainLoop(Core& core);
		void run(bool capFrameRate, int targetFps);

	private:
		Core& core;
		unsigned int delay = 0;

		void onVariableUpdate(Time delta);
		void onFixedUpdate(Time delta);
		bool isRunning() const;
	};
}
