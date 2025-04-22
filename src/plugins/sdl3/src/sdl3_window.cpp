#include "sdl3_window.h"
#include "halley/os/os.h"
#include "halley/game/game_platform.h"

using namespace Halley;

SDL3Window::SDL3Window(SDL_Window* window, bool owning)
	: window(window)
	, owning(owning)
{
#ifdef _WIN32
    OS::get().onWindowCreated(getNativeHandle());
#endif

    displays = SDL_GetDisplays(&numDisplays);

    if (displays == nullptr) {
        displays = (SDL_DisplayID*) SDL_aligned_alloc(4, 1);
        displays[0] = SDL_GetPrimaryDisplay();
        numDisplays = 1;
    }
}

SDL3Window::~SDL3Window()
{
    SDL_free(displays);
}

Vector2i SDL3Window::getCenteredWindow(Vector2i size, int screen) const
{
	screen = std::max(0, std::min(screen, numDisplays - 1));

	SDL_Rect rect;
	SDL_GetDisplayBounds(displays[screen], &rect);

    return {
            rect.x + (rect.w - size.x) / 2,
            rect.y + (rect.h - size.y) / 2
    };
}

void SDL3Window::update(const WindowDefinition& definition)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
	return;
#endif

	// Update window position & size
	// For windowed, get out of fullscreen first, then set size.
	// For fullscreen, set size before going to fullscreen.
	const WindowType windowType = definition.getWindowType();
	const WindowState windowState = definition.getWindowState();
	const Vector2i windowSize = definition.getSize();
	const Vector2i windowPos = definition.getPosition().value_or(getCenteredWindow(windowSize, definition.getScreen()));
	const std::optional<Path> icon = definition.getIcon();

	if (windowType != WindowType::Fullscreen) {
		SDL_SetWindowFullscreen(window, 0);
	}
	SDL_SetWindowSize(window, windowSize.x, windowSize.y);
	if (windowType == WindowType::Fullscreen) {
    	SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
	}
	SDL_SetWindowBordered(window, windowType == WindowType::ResizableWindow || windowType == WindowType::Window);

#ifndef __APPLE__ // Hack until I update SDL2 to 2.0.7 on Mac
#if SDL_MAJOR_VERSION > 2 || SDL_MINOR_VERSION > 0 || SDL_PATCHLEVEL >= 5
	SDL_SetWindowResizable(window, windowType == WindowType::ResizableWindow);
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
        SDL_DestroySurface(surface);
	}

	updateDefinition(definition);
}

void SDL3Window::updateDefinition(const WindowDefinition& definition)
{
	curDefinition = std::make_unique<WindowDefinition>(definition);
}

void SDL3Window::show()
{
	SDL_ShowWindow(window);
}

void SDL3Window::hide()
{
	SDL_HideWindow(window);
}

void SDL3Window::setVsync(bool vsync)
{
	SDL_GL_SetSwapInterval(vsync ? 1 : 0);
}

void SDL3Window::swap()
{
	SDL_GL_SwapWindow(window);
}

uint32_t SDL3Window::getId() const
{
	return SDL_GetWindowID(window);
}

void SDL3Window::resize(Rect4i windowSize)
{
	update(curDefinition->withPosition(windowSize.getTopLeft()).withSize(windowSize.getSize()));
}

void SDL3Window::destroy()
{
	if (owning && window) {
		SDL_HideWindow(window);
		SDL_DestroyWindow(window);
	}
	window = nullptr;
}

void* SDL3Window::getNativeHandle() const
{
#ifdef _WIN32
    return SDL_GetPointerProperty(SDL_GetWindowProperties(window), "SDL.window.win32.hwnd", nullptr);
#elif __APPLE__
    return window;
#else
    return nullptr;
#endif
}

String SDL3Window::getNativeHandleType() const
{
#ifdef _WIN32
	return "HWND";
#elif __APPLE__
    return "SDL";
#else
	return "";
#endif
}

Rect4i SDL3Window::getWindowRect() const
{
	int x, y, w, h;
	SDL_GetWindowPosition(window, &x, &y);
	SDL_GetWindowSize(window, &w, &h);
    return {x, y, w, h};
}

void SDL3Window::setTitleColour(Colour4f bgCol, Colour4f textCol)
{
    // Not implemented yet.
    // DWM doesn't seem to be available with WINAPI_FAMILY_GAMES.
}
