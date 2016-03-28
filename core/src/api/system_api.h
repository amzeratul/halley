#pragma once

namespace Halley
{
	class VideoAPI;
	class InputAPI;
	class HalleyAPIInternal;

	class SystemAPI
	{
	public:
		virtual ~SystemAPI() {}

		virtual unsigned int getTicks() = 0;
		virtual void delay(unsigned int ms) = 0;

	private:
		friend class HalleyAPI;
		friend class CoreRunner;

		virtual bool generateEvents(HalleyAPIInternal* video, HalleyAPIInternal* input) = 0;
	};
}