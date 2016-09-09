#pragma once

#include "halley/core/api/halley_api_internal.h"
#include "../../opengl/src/native_window.h"

namespace Halley
{
	class SystemSDL final : public SystemAPIInternal
	{
	public:
		unsigned int getTicks() override;
		void delay(unsigned int ms) override;

	protected:
		friend class HalleyAPI;
		friend class Core;

		void init() override;
		void deInit() override;

		bool generateEvents(VideoAPI* video, InputAPI* input) override;

		std::unique_ptr<InputAPIInternal> makeInputAPI() override;

	private:
		void processVideoEvent(VideoAPI* video, const SDL_Event& event);
	};
}
