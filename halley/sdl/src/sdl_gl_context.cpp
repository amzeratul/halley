#include "sdl_gl_context.h"

using namespace Halley;

SDLGLContext::SDLGLContext(SDL_Window* window)
	: window(window)
	, owner(true)
{
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
	sharedContext = SDL_GL_CreateContext(window);
	context = SDL_GL_CreateContext(window);
}

SDLGLContext::SDLGLContext(SDL_Window* window, SDL_GLContext shared)
	: window(window)
	, context(shared)
	, sharedContext(nullptr)
	, owner(false)
{
}

SDLGLContext::~SDLGLContext()
{
	SDL_GL_MakeCurrent(window, nullptr);
	if (owner) {
		SDL_GL_DeleteContext(sharedContext);
		SDL_GL_DeleteContext(context);
	}
}

void SDLGLContext::bind()
{
	SDL_GL_MakeCurrent(window, context);
}

std::unique_ptr<GLContext> SDLGLContext::createSharedContext()
{
	return std::make_unique<SDLGLContext>(window, sharedContext);
}
