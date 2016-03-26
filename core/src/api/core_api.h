#pragma once

namespace Halley
{
	class VideoAPI;
	class InputAPI;

	class CoreAPI
	{
	public:
		unsigned int getTicks();
		void delay(unsigned int ms);

	private:
		friend class HalleyAPI;
		friend class CoreRunner;

		void init();
		void deInit();
		bool processEvents(VideoAPI* video, InputAPI* input);
	};
}