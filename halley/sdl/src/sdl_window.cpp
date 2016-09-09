#include "sdl_window.h"
#include "halley/os/os.h"
#include <SDL_syswm.h>

// win32 crap
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

using namespace Halley;


SDLWindow::SDLWindow(SDL_Window* window)
	: window(window)
{
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	if (SDL_GetWindowWMInfo(window, &wminfo) == 1) {
		OS::get().onWindowCreated(wminfo.info.win.window);
	}
}

static Vector2i getCenteredWindow(Vector2i size, int screen)
{
	screen = std::max(0, std::min(screen, SDL_GetNumVideoDisplays() - 1));

	SDL_Rect rect;
	SDL_GetDisplayBounds(screen, &rect);
	auto recti = Rect4i(rect.x, rect.y, rect.w, rect.h);
	return recti.getTopLeft() + (recti.getSize() - size) / 2;
}

void SDLWindow::update(const WindowDefinition& definition)
{
#ifdef __ANDROID__
	return;
#endif

	// Update window position & size
	// For windowed, get out of fullscreen first, then set size.
	// For fullscreen, set size before going to fullscreen.
	WindowType windowType = definition.getWindowType();
	Vector2i windowSize = definition.getSize();
	Vector2i windowPos = definition.getPosition().get_value_or(getCenteredWindow(windowSize, 0));

	if (windowType != WindowType::Fullscreen) {
		SDL_SetWindowFullscreen(window, SDL_FALSE);
	}
	SDL_SetWindowSize(window, windowSize.x, windowSize.y);
	if (windowType == WindowType::Fullscreen) {
		SDL_SetWindowFullscreen(window, SDL_TRUE);
	}

	SDL_SetWindowPosition(window, windowPos.x, windowPos.y);

	curDefinition = std::make_unique<WindowDefinition>(definition);
}

void SDLWindow::show()
{
	SDL_ShowWindow(window);
}

void SDLWindow::hide()
{
	SDL_HideWindow(window);
}

void SDLWindow::setVsync(bool vsync)
{
	SDL_GL_SetSwapInterval(vsync ? 1 : 0);
}

void SDLWindow::swap()
{
	SDL_GL_SwapWindow(window);
}

int SDLWindow::getId() const
{
	return SDL_GetWindowID(window);
}

void SDLWindow::resize(Rect4i windowSize)
{
	update(curDefinition->withPosition(windowSize.getTopLeft()).withSize(windowSize.getSize()));
}

void SDLWindow::destroy()
{
	SDL_HideWindow(window);
	SDL_DestroyWindow(window);
	window = nullptr;
}

Rect4i SDLWindow::getWindowRect() const
{
	int x, y, w, h;
	SDL_GetWindowPosition(window, &x, &y);
	SDL_GetWindowSize(window, &w, &h);
	return Rect4i(x, y, w, h);
}
