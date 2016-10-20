#pragma once

#include "halley/core/api/halley_api_internal.h"
#include <SDL.h>

namespace Halley
{
	class SDLWindow;

	class SystemSDL final : public SystemAPIInternal
	{
	protected:
		void init() override;
		void deInit() override;

		bool generateEvents(VideoAPI* video, InputAPI* input) override;

		std::unique_ptr<ResourceDataReader> getDataReader(String path, int64_t start, int64_t end) override;
		std::unique_ptr<ResourceDataReader> getDataReader(gsl::span<const gsl::byte> memory) override;

		std::shared_ptr<Window> createWindow(const WindowDefinition& window) override;
		void destroyWindow(std::shared_ptr<Window> window) override;

		Vector2i getScreenSize(int n) const override;
		Rect4i getDisplayRect(int screen) const override;
		Vector2i getCenteredWindow(Vector2i size, int screen) const;
		std::unique_ptr<GLContext> createGLContext() override;

		void showCursor(bool show) override;

	private:
		void processVideoEvent(VideoAPI* video, const SDL_Event& event);

		void printDebugInfo() const;

		void initVideo();
		void deInitVideo();

		std::vector<std::shared_ptr<SDLWindow>> windows;
		bool videoInit = false;
	};
}
