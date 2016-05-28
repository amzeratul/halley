#pragma once

namespace Halley
{
	class Core;

	class MainLoop
	{
	public:
		void run(Core& core, bool capFrameRate, int targetFps);

	private:
		unsigned int delay = 0;
	};
}