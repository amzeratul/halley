#pragma once

#include "halley/core/api/halley_api_internal.h"
#include "../../opengl/src/native_window.h"

namespace Halley
{
	class SystemSDL final : public SystemAPIInternal
	{
	protected:
		friend class HalleyAPI;
		friend class Core;

		void init() override;
		void deInit() override;

		bool generateEvents(VideoAPI* video, InputAPI* input) override;

		std::unique_ptr<InputAPIInternal> makeInputAPI() override;

		std::unique_ptr<ResourceDataReader> getDataReader(String path, int64_t start, int64_t end) override;
		std::unique_ptr<ResourceDataReader> getDataReader(gsl::span<const gsl::byte> memory) override;

	private:
		void processVideoEvent(VideoAPI* video, const SDL_Event& event);
	};
}
