#pragma once
#include "halley/core/graphics/window.h"
#include <SDL.h>

namespace Halley
{
	class SDLWindow : public Window
	{
	public:
		SDLWindow(SDL_Window* window);

		void update(const WindowDefinition& definition) override;
		void show() override;
		void hide() override;
		void setVsync(bool vsync) override;
		void swap() override;
		Rect4i getWindowRect() const override;

		int getId() const;
		void resize(Rect4i size);

		SDL_Window* getSDLWindow() const { return window; }
		const WindowDefinition& getDefinition() const override { return *curDefinition; }

		void destroy();

		void* getNativeHandle() override;
		String getNativeHandleType() override;

	private:
		SDL_Window* window;
		std::unique_ptr<WindowDefinition> curDefinition;
	};
}
