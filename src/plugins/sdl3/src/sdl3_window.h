#pragma once
#include "halley/graphics/window.h"
#include <SDL3/SDL.h>

namespace Halley
{
	class SDL3Window final : public Window
	{
	public:
		explicit SDL3Window(SDL_Window* window, bool owning = true);
        ~SDL3Window() override;

		void update(const WindowDefinition& definition) override;
		void updateDefinition(const WindowDefinition& definition);

		void show() override;
		void hide() override;
		void setVsync(bool vsync) override;
		void swap() override;
		Rect4i getWindowRect() const override;

		uint32_t getId() const;
		void resize(Rect4i size);

		SDL_Window* getSDLWindow() const { return window; }
		const WindowDefinition& getDefinition() const override { return *curDefinition; }

		void destroy();

		void* getNativeHandle() const override;
		String getNativeHandleType() const override;

		void setTitleColour(Colour4f bgCol, Colour4f textCol) override;

	private:
		SDL_Window* window;
        SDL_DisplayID* displays;
        int numDisplays;
		bool owning;
		std::unique_ptr<WindowDefinition> curDefinition;

        Vector2i getCenteredWindow(Vector2i size, int screen) const;
	};
}
