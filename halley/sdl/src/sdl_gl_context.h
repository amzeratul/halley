#pragma once
#include "halley/core/api/system_api.h"
#include <SDL.h>

namespace Halley
{
	class SDLGLContext : public GLContext
	{
	public:
		SDLGLContext(SDL_Window* window);
		~SDLGLContext();
		void bind() override;

	private:
		SDL_Window* window;
		SDL_GLContext context;
	};
}
