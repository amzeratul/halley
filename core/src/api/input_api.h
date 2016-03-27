#pragma once

union SDL_Event;

namespace Halley
{
	class InputAPI
	{
	public:

	private:
		friend class HalleyAPI;
		friend class SystemAPI;

		void init();
		void deInit();
		void processEvent(SDL_Event& event);
	};
}
