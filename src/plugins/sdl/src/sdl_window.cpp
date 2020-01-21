#include "sdl_window.h"
#include "halley/os/os.h"
#include <SDL_syswm.h>
#include "halley/core/game/game_platform.h"

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
#ifdef _WIN32
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	if (SDL_GetWindowWMInfo(window, &wminfo) == 1) {
		OS::get().onWindowCreated(wminfo.info.win.window);
	}
#endif
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
	const WindowType windowType = definition.getWindowType();
	const WindowState windowState = definition.getWindowState();
	const Vector2i windowSize = definition.getSize();
	const Vector2i windowPos = definition.getPosition().get_value_or(getCenteredWindow(windowSize, 0));
	const Maybe<Path> icon = definition.getIcon();

	if (windowType != WindowType::Fullscreen) {
		SDL_SetWindowFullscreen(window, 0);
	}
	SDL_SetWindowSize(window, windowSize.x, windowSize.y);
	if (windowType == WindowType::Fullscreen) {
		if (getPlatform() == GamePlatform::MacOS) {
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		} else {
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
		}
	}
	SDL_SetWindowBordered(window, windowType == WindowType::ResizableWindow || windowType == WindowType::Window ? SDL_TRUE : SDL_FALSE);

#ifndef __APPLE__ // Hack until I update SDL2 to 2.0.7 on Mac
#if SDL_MAJOR_VERSION > 2 || SDL_MINOR_VERSION > 0 || SDL_PATCHLEVEL >= 5
	SDL_SetWindowResizable(window, windowType == WindowType::ResizableWindow ? SDL_TRUE : SDL_FALSE);
#endif
#endif

	SDL_SetWindowPosition(window, windowPos.x, windowPos.y);

	switch (windowState) {
	case WindowState::Normal:
		SDL_RestoreWindow(window);
		break;
	case WindowState::Minimized:
		SDL_MinimizeWindow(window);
		break;
	case WindowState::Maximized:
		SDL_MaximizeWindow(window);
		break;
	}

	if (icon) {
		auto surface = SDL_LoadBMP(icon->string().c_str());
		SDL_SetWindowIcon(window, surface);
		SDL_FreeSurface(surface);
	}

	updateDefinition(definition);
}

void SDLWindow::updateDefinition(const WindowDefinition& definition)
{
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

void* SDLWindow::getNativeHandle()
{
#ifdef _WIN32
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	if (SDL_GetWindowWMInfo(window, &wminfo) == 1) {
		return wminfo.info.win.window;
	}
#elif __APPLE__
  return window;
#endif
	return nullptr;
}

String SDLWindow::getNativeHandleType()
{
#ifdef _WIN32
	return "HWND";
#elif __APPLE__
  return "SDL";
#else
	return "";
#endif
}

Rect4i SDLWindow::getWindowRect() const
{
	int x, y, w, h;
	SDL_GetWindowPosition(window, &x, &y);
	SDL_GetWindowSize(window, &w, &h);
	return Rect4i(x, y, w, h);
}
