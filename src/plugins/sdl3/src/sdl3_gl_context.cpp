#include "sdl3_gl_context.h"

using namespace Halley;

SDL3GLContext::SDL3GLContext(SDL_Window* window)
	: window(window)
	, owner(true)
{
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
	sharedContext = SDL_GL_CreateContext(window);
	context = SDL_GL_CreateContext(window);
}

SDL3GLContext::SDL3GLContext(SDL_Window* window, ContextType shared)
	: window(window)
	, context(shared)
	, sharedContext(ContextType{})
	, owner(false)
{
}

SDL3GLContext::~SDL3GLContext()
{
	SDL_GL_MakeCurrent(window, nullptr);
	if (owner) {
		SDL_GL_DestroyContext(sharedContext);
		SDL_GL_DestroyContext(context);
	}
}

void SDL3GLContext::bind()
{
	const int error = SDL_GL_MakeCurrent(window, context);
	if (error) {
		Logger::logError("Error binding SDL GL Context: " + toString(SDL_GetError()));
	}
}

std::unique_ptr<GLContext> SDL3GLContext::createSharedContext()
{
	return std::make_unique<SDL3GLContext>(window, sharedContext);
}

void* SDL3GLContext::getGLProcAddress(const char* name)
{
	return (void*) SDL_GL_GetProcAddress(name);
}
