#pragma once

#include "halley/api/system_api.h"
#include <SDL3/SDL.h>

using ContextType = SDL_GLContext;

namespace Halley
{
	class SDL3GLContext : public GLContext
	{
	public:
		explicit SDL3GLContext(SDL_Window* window);
		SDL3GLContext(SDL_Window* window, ContextType shared);
		~SDL3GLContext() override;

		void bind() override;
		std::unique_ptr<GLContext> createSharedContext() override;
		void* getGLProcAddress(const char* name) override;

	private:
		SDL_Window* window;
		ContextType context;
		ContextType sharedContext;
		bool owner;
	};
}
