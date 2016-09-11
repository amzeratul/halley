#pragma once
#include "halley/core/api/system_api.h"
#include <SDL.h>

namespace Halley
{
	class SDLGLContext : public GLContext
	{
	public:
		SDLGLContext(SDL_Window* window);
		SDLGLContext(SDL_Window* window, SDL_GLContext shared);
		~SDLGLContext();

		void bind() override;
		std::unique_ptr<GLContext> createSharedContext() override;

	private:
		SDL_Window* window;
		SDL_GLContext context;
		SDL_GLContext sharedContext;
		bool owner;
	};
}
