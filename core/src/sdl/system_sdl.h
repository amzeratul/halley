#pragma once

#include "../api/halley_api_internal.h"

namespace Halley
{
	class SystemSDL : public SystemAPIInternal
	{
	public:
		unsigned int getTicks() override;
		void delay(unsigned int ms) override;

	protected:
		friend class HalleyAPI;
		friend class CoreRunner;

		void init() override;
		void deInit() override;
		void processEvent(SDL_Event& evt) override;

		bool generateEvents(HalleyAPIInternal* video, HalleyAPIInternal* input) override;
	};
}
