#include "system_sdl.h"
#include <SDL.h>
#include <SDL_syswm.h>
#include "halley/core/api/halley_api_internal.h"
#include <halley/support/console.h>
#include <halley/support/exception.h>
#include "input_sdl.h"
#include "sdl_rw_ops.h"
#include "halley/core/graphics/window.h"
#include "halley/os/os.h"

// win32 crap
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

using namespace Halley;

#ifdef _MSC_VER
#pragma comment(lib, "SDL2.lib")
#endif

void SystemSDL::init()
{
	// SDL version
	SDL_version compiled;
	SDL_version linked;
	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	std::cout << ConsoleColour(Console::GREEN) << "\nInitializing SDL..." << ConsoleColour() << std::endl;
	std::cout << "\tVersion/Compiled: " << ConsoleColour(Console::DARK_GREY) << int(compiled.major) << "." << int(compiled.minor) << "." << int(compiled.patch) << ConsoleColour() << std::endl;
	std::cout << "\tVersion/Linked: " << ConsoleColour(Console::DARK_GREY) << int(linked.major) << "." << int(linked.minor) << "." << int(linked.patch) << ConsoleColour() << std::endl;

	// Initialize SDL
	if (!SDL_WasInit(SDL_INIT_NOPARACHUTE)) {
		if (SDL_Init(SDL_INIT_NOPARACHUTE) == -1) {
			throw Exception(String("Exception initializing SDL: ") + SDL_GetError());
		}
	}
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1) {
		throw Exception(String("Exception initializing video: ") + SDL_GetError());
	}
	if (SDL_InitSubSystem(SDL_INIT_TIMER) == -1) {
		throw Exception(String("Exception initializing timer: ") + SDL_GetError());
	}
	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1) {
		throw Exception(String("Exception initializing joystick: ") + SDL_GetError());
	}

	//SDL_EnableUNICODE(1);
	//SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_ShowCursor(SDL_DISABLE);
}

void SystemSDL::deInit()
{
	// Close SDL
	SDL_Quit();
}

bool SystemSDL::generateEvents(VideoAPI* video, InputAPI* input)
{
	SDL_Event event;
	SDL_PumpEvents();
	while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) > 0) {
		switch (event.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			case SDL_TEXTINPUT:
			case SDL_TEXTEDITING:
			case SDL_JOYAXISMOTION:
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
			case SDL_JOYHATMOTION:
			case SDL_JOYBALLMOTION:
			case SDL_MOUSEMOTION:
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_FINGERUP:
			case SDL_FINGERDOWN:
			case SDL_FINGERMOTION:
			{
				auto sdlInput = dynamic_cast<InputSDL*>(input);
				if (sdlInput) {
					sdlInput->processEvent(event);
				}
				break;
			}
			case SDL_QUIT:
			{
				std::cout << "SDL_QUIT received." << std::endl;
				return false;
			}
			case SDL_WINDOWEVENT:
			{
				if (video) {
					processVideoEvent(video, event);
				}
			}
		}
	}
	return true;
}

std::unique_ptr<InputAPIInternal> SystemSDL::makeInputAPI()
{
	return std::make_unique<InputSDL>();
}

void SystemSDL::processVideoEvent(VideoAPI* video, const SDL_Event& event)
{
	if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
		int x, y;
		SDL_GetWindowPosition(SDL_GetWindowFromID(event.window.windowID), &x, &y);
		resizeWindow(Rect4i(x, y, event.window.data1, event.window.data2));
	}
}

std::unique_ptr<ResourceDataReader> SystemSDL::getDataReader(String path, int64_t start, int64_t end)
{
	try {
		return SDLRWOps::fromPath(path, start, end);
	} catch (...) {
		return std::unique_ptr<ResourceDataReader>();
	}
}

std::unique_ptr<ResourceDataReader> SystemSDL::getDataReader(gsl::span<const gsl::byte> memory)
{
	try {
		return SDLRWOps::fromMemory(memory);
	} catch (...) {
		return std::unique_ptr<ResourceDataReader>();
	}
}

void* SystemSDL::createWindow(const WindowDefinition& window)
{
	// Set flags and GL attributes
	auto windowType = window.getWindowType();
	int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_GRABBED | SDL_WINDOW_ALLOW_HIGHDPI;
	if (windowType == WindowType::BorderlessWindow) {
		flags |= SDL_WINDOW_BORDERLESS;
	}
	else if (windowType == WindowType::ResizableWindow) {
		flags |= SDL_WINDOW_RESIZABLE;
	}
	else if (windowType == WindowType::Fullscreen) {
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	// Context options
#if defined(WITH_OPENGL_ES2)
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 0);
#else
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#ifdef _DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

	// Window position
	Vector2i windowSize = window.getSize();
	Vector2i winPos = window.getPosition().get_value_or(getCenteredWindow(windowSize, 0));

	// Create window
	sdlWindow = SDL_CreateWindow(window.getTitle().c_str(), winPos.x, winPos.y, windowSize.x, windowSize.y, flags);
	if (!sdlWindow) {
		throw Exception(String("Error creating SDL window: ") + SDL_GetError());
	}

	// Set window icon
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	if (SDL_GetWindowWMInfo(sdlWindow, &wminfo) == 1) {
		OS::get().onWindowCreated(wminfo.info.win.window);
	}

	// Show window
	SDL_ShowWindow(sdlWindow);

	updateWindow(window);

	return sdlWindow;
}

void SystemSDL::destroyWindow(std::shared_ptr<Window> window)
{
	
}

void SystemSDL::updateWindow(const WindowDefinition& window)
{
#ifdef __ANDROID__
	return;
#endif

	// Update window position & size
	// For windowed, get out of fullscreen first, then set size.
	// For fullscreen, set size before going to fullscreen.
	WindowType windowType = window.getWindowType();
	Vector2i windowSize = window.getSize();
	Vector2i windowPos = window.getPosition().get_value_or(getCenteredWindow(windowSize, 0));

	if (windowType != WindowType::Fullscreen) {
		SDL_SetWindowFullscreen(sdlWindow, SDL_FALSE);
	}
	SDL_SetWindowSize(sdlWindow, windowSize.x, windowSize.y);
	if (windowType == WindowType::Fullscreen) {
		SDL_SetWindowFullscreen(sdlWindow, SDL_TRUE);
	}

	SDL_SetWindowPosition(sdlWindow, windowPos.x, windowPos.y);

	curWindow = std::make_unique<WindowDefinition>(window);
}

void SystemSDL::resizeWindow(Rect4i windowSize)
{
	updateWindow(curWindow->withPosition(windowSize.getTopLeft()).withSize(windowSize.getSize()));
}

Vector2i SystemSDL::getScreenSize(int n) const
{
	if (n >= SDL_GetNumVideoDisplays()) {
		return Vector2i();
	}
	SDL_DisplayMode info;
	SDL_GetDesktopDisplayMode(n, &info);
	return Vector2i(info.w, info.h);
}

Rect4i SystemSDL::getWindowRect() const
{
	int x, y, w, h;
	SDL_GetWindowPosition(sdlWindow, &x, &y);
	SDL_GetWindowSize(sdlWindow, &w, &h);
	return Rect4i(x, y, w, h);
}

Rect4i SystemSDL::getDisplayRect(int screen) const
{
	screen = std::max(0, std::min(screen, SDL_GetNumVideoDisplays() - 1));

	SDL_Rect rect;
	SDL_GetDisplayBounds(screen, &rect);
	return Rect4i(rect.x, rect.y, rect.w, rect.h);
}

Vector2i SystemSDL::getCenteredWindow(Vector2i size, int screen) const
{
	Rect4i rect = getDisplayRect(screen);
	return rect.getTopLeft() + (rect.getSize() - size) / 2;
}

std::unique_ptr<GLContext> SystemSDL::createGLContext()
{
	// TODO
}

void SystemSDL::printDebugInfo() const
{
	std::cout << std::endl << ConsoleColour(Console::GREEN) << "Initializing Video Display...\n" << ConsoleColour();
	std::cout << "Drivers available:\n";
	for (int i = 0; i < SDL_GetNumVideoDrivers(); i++) {
		std::cout << "\t" << i << ": " << SDL_GetVideoDriver(i) << "\n";
	}
	std::cout << "Video driver: " << ConsoleColour(Console::DARK_GREY) << SDL_GetCurrentVideoDriver() << ConsoleColour() << std::endl;
}
