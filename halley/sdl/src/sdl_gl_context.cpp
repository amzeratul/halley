#include "sdl_gl_context.h"

using namespace Halley;

SDLGLContext::SDLGLContext(SDL_Window* window)
	: window(window)
{
	context = SDL_GL_CreateContext(window);
}

SDLGLContext::~SDLGLContext()
{
	SDLGLContext::bind();
	SDL_GL_DeleteContext(context);
}

void SDLGLContext::bind()
{
	SDL_GL_MakeCurrent(window, context);
}
