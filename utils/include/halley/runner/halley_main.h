#pragma once

#include <halley/runner/game_loader.h>
#include <halley/text/halleystring.h>
#include <halley/support/console.h>
#include <halley/core/game/core.h>
#include "main_loop.h"
#include "entry_point.h"

namespace Halley
{
	class Game;

	class HalleyMain {
	public:
		template <typename T>
		static int main(int argc, char* argv[])
		{
			StringArray args;
			for (int i = 0; i < argc; i++) {
				args.push_back(argv[i]);
			}
			StaticGameLoader<T> reloader;
			return runMain(reloader, args);
		}

		static int runMain(GameLoader& loader, const StringArray& args)
		{
			try {
				Core core(loader.createGame(), args);
				loader.setCore(core);
				core.init();
				MainLoop loop(core, loader);
				loop.run();
				return 0;
			}
			catch (std::exception& e) {
				std::cout << ConsoleColor(Console::RED) << "\n\nUnhandled exception: " << ConsoleColor(Console::DARK_RED) << e.what() << ConsoleColor() << std::endl;
				return 1;
			}
			catch (...) {
				std::cout << ConsoleColor(Console::RED) << "\n\nUnknown unhandled exception." << ConsoleColor() << std::endl;
				return 1;
			}
		}
	};
}

// Macro to implement program
#define HalleyGame(T) int main(int argc, char* argv[]) { return Halley::HalleyMain::main<T>(argc, argv); }

#ifdef _WIN32
	#define HALLEY_EXPORT extern "C" __declspec(dllexport)
	#pragma comment(lib, "SDL2.lib")
#else
	#define HALLEY_EXPORT
#endif
