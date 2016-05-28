/*

}
catch (std::exception& e) {
handleException(e);
return 1;
}
catch (...) {
std::cout << ConsoleColor(Console::RED) << "Unknown unhandled exception in initialization" << ConsoleColor() << std::endl;
//else std::cout << ConsoleColor(Console::RED) << "Unknown unhandled exception in main loop" << ConsoleColor() << std::endl;
crashed = true;
return 1;
}

*/

/*
try {
StringArray args;
for (int i = 0; i < argc; i++) {
args.push_back(argv[i]);
}
Core runner(std::make_unique<T>(), args);
return 0;
} catch (std::exception& e) {
std::cout << "Unhandled std::exception: " << e.what() << std::endl;
return 1;
} catch (...) {
std::cout << "Unhandled unknown exception." << std::endl;
return 1;
}

*/

#include "game/halley_main.h"
#include "game/main_loop.h"

using namespace Halley;

int HalleyMain::staticMain(std::unique_ptr<Game> game, const StringArray& args)
{
	try {
		Core core(std::move(game), args);
		MainLoop loop(core);
		loop.run(true, 60);
		return 0;
	} catch (std::exception& e) {
		std::cout << ConsoleColor(Console::RED) << "\n\nUnhandled exception: " << ConsoleColor(Console::DARK_RED) << e.what() << ConsoleColor() << std::endl;
		return 1;
	} catch (...) {
		std::cout << ConsoleColor(Console::RED) << "\n\nUnknown unhandled exception." << ConsoleColor() << std::endl;
		return 1;
	}
}
