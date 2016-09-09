#include "system_sdl.h"
#include <SDL.h>
#include "halley/core/api/halley_api_internal.h"
#include <halley/support/console.h>
#include <halley/support/exception.h>
#include "input_sdl.h"

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

unsigned int SystemSDL::getTicks()
{
	return SDL_GetTicks();
}

void SystemSDL::delay(unsigned int ms)
{
	SDL_Delay(ms);
}

void SystemSDL::processEvent(SDL_Event& evt)
{
}

bool SystemSDL::generateEvents(HalleyAPIInternal* video, HalleyAPIInternal* input)
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
			if (input) {
				// TODO: convert to internal event format?
				input->processEvent(event);
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
				// TODO: convert to internal event format?
				video->processEvent(event);
			}
			break;
		}
		}
	}
	return true;
}

std::unique_ptr<InputAPIInternal> SystemSDL::makeInputAPI()
{
	return std::make_unique<InputSDL>();
}
