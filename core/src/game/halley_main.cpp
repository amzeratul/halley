#include "game/halley_main.h"
#include "game/main_loop.h"
#include "game/dynamic_reloader.h"

using namespace Halley;

int HalleyMain::staticMain(std::unique_ptr<Game> game, const StringArray& args)
{
	try {
		Core core(std::move(game), args);
		GameReloader reloader;
		MainLoop loop(core, reloader);
		loop.run();
		return 0;
	} catch (std::exception& e) {
		std::cout << ConsoleColor(Console::RED) << "\n\nUnhandled exception: " << ConsoleColor(Console::DARK_RED) << e.what() << ConsoleColor() << std::endl;
		return 1;
	} catch (...) {
		std::cout << ConsoleColor(Console::RED) << "\n\nUnknown unhandled exception." << ConsoleColor() << std::endl;
		return 1;
	}
}

int HalleyMain::dynamicMain(String dllName, const StringArray& args)
{
	try {
		DynamicGameReloader reloader(dllName);
		Core core(reloader.createGame(), args);
		MainLoop loop(core, reloader);
		loop.run();
		return 0;
	} catch (std::exception& e) {
		std::cout << ConsoleColor(Console::RED) << "\n\nUnhandled exception: " << ConsoleColor(Console::DARK_RED) << e.what() << ConsoleColor() << std::endl;
		return 1;
	} catch (...) {
		std::cout << ConsoleColor(Console::RED) << "\n\nUnknown unhandled exception." << ConsoleColor() << std::endl;
		return 1;
	}
}
