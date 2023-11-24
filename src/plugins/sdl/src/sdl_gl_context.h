#pragma once
#include "halley/api/system_api.h"
#include <SDL.h>

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
using ContextType = EMSCRIPTEN_WEBGL_CONTEXT_HANDLE;
#else
using ContextType = SDL_GLContext;
#endif


namespace Halley
{
	class SDLGLContext : public GLContext
	{
	public:
		SDLGLContext(SDL_Window* window);
		SDLGLContext(SDL_Window* window, ContextType shared);
		~SDLGLContext();

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
