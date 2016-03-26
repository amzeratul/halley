#include "../api/core_api.h"
#include <SDL.h>

using namespace Halley;

#ifdef _MSC_VER
#pragma comment(lib, "SDL2.lib")
#endif

void CoreAPI::init()
{
	// SDL version
	SDL_version compiled;
	SDL_version linked;
	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	std::cout << ConsoleColor(Console::GREEN) << "\nInitializing SDL..." << ConsoleColor() << std::endl;
	std::cout << "\tVersion/Compiled: " << ConsoleColor(Console::DARK_GREY) << int(compiled.major) << "." << int(compiled.minor) << "." << int(compiled.patch) << ConsoleColor() << std::endl;
	std::cout << "\tVersion/Linked: " << ConsoleColor(Console::DARK_GREY) << int(linked.major) << "." << int(linked.minor) << "." << int(linked.patch) << ConsoleColor() << std::endl;

	// Initialize SDL
	if (SDL_Init(SDL_INIT_NOPARACHUTE) == -1)
		throw Exception(String("Exception initializing SDL: ") + SDL_GetError());
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
		throw Exception(String("Exception initializing video: ") + SDL_GetError());
	if (SDL_InitSubSystem(SDL_INIT_TIMER) == -1)
		throw Exception(String("Exception initializing timer: ") + SDL_GetError());
	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1)
		throw Exception(String("Exception initializing joystick: ") + SDL_GetError());

	//SDL_EnableUNICODE(1);
	//SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_ShowCursor(SDL_DISABLE);
}

void CoreAPI::deInit()
{
	// Close SDL
	SDL_Quit();
}

unsigned int CoreAPI::getTicks()
{
	return SDL_GetTicks();
}

void CoreAPI::delay(unsigned int ms)
{
	SDL_Delay(ms);
}
