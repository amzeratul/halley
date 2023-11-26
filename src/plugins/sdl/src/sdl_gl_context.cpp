#include "sdl_gl_context.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/threading.h>
#endif

using namespace Halley;

SDLGLContext::SDLGLContext(SDL_Window* window)
	: window(window)
	, owner(true)
{
#ifdef __EMSCRIPTEN__
	EmscriptenWebGLContextAttributes attr;
	emscripten_webgl_init_context_attributes(&attr);
	attr.majorVersion = 3;
	attr.minorVersion = 0;
	attr.stencil = true;
	attr.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_ALWAYS;
	attr.renderViaOffscreenBackBuffer = true;
	attr.explicitSwapControl = true;
	attr.premultipliedAlpha = false;
	context = emscripten_webgl_create_context("#canvas", &attr);
	if (context < 0) {
		Logger::logError("Failed to initialize Emscripten WebGL context with error " + toString(int(context)));
	}
	emscripten_webgl_make_context_current(context);
	sharedContext = context;
#else
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
	sharedContext = SDL_GL_CreateContext(window);
	context = SDL_GL_CreateContext(window);
#endif
}

SDLGLContext::SDLGLContext(SDL_Window* window, ContextType shared)
	: window(window)
	, context(shared)
	, sharedContext(ContextType{})
	, owner(false)
{
}

SDLGLContext::~SDLGLContext()
{
#ifdef __EMSCRIPTEN__
	emscripten_webgl_make_context_current(0);
	if (owner) {
		emscripten_webgl_destroy_context(context);
	}
#else
	SDL_GL_MakeCurrent(window, nullptr);
	if (owner) {
		SDL_GL_DeleteContext(sharedContext);
		SDL_GL_DeleteContext(context);
	}
#endif
}

void SDLGLContext::bind()
{
#ifdef __EMSCRIPTEN__
	const auto error = emscripten_webgl_make_context_current(context);
	if (error) {
		Logger::logError("Error binding Emscripten WebGL context " + toString(int(context)) + ", code: " + toString(int(error)));
	}
#else
	const int error = SDL_GL_MakeCurrent(window, context);
	if (error) {
		Logger::logError("Error binding SDL GL Context: " + toString(SDL_GetError()));
	}
#endif
}

std::unique_ptr<GLContext> SDLGLContext::createSharedContext()
{
	return std::make_unique<SDLGLContext>(window, sharedContext);
}

void* SDLGLContext::getGLProcAddress(const char* name)
{
	return SDL_GL_GetProcAddress(name);
}
