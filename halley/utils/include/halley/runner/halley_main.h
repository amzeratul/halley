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
			Vector<std::string> args;
			for (int i = 0; i < argc; i++) {
				args.push_back(argv[i]);
			}
			StaticGameLoader<T> reloader;
			return runMain(reloader, args);
		}

		static int runMain(GameLoader& loader, const Vector<std::string>& args)
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

#ifdef _MSC_VER
	#define HALLEY_EXPORT extern "C" __declspec(dllexport)
	#pragma comment(lib, "SDL2.lib")
	#if HAS_EASTL
		#ifndef _DEBUG
			#pragma comment(lib, "EASTL.lib")
		#else
			#pragma comment(lib, "EASTLd.lib")
		#endif
	#endif
#else
	#define HALLEY_EXPORT
#endif

// Macro to implement program
#if defined(HALLEY_EXECUTABLE)
	#define HalleyGame(T) int main(int argc, char* argv[]) { return Halley::HalleyMain::main<T>(argc, argv); }
#elif defined(HALLEY_SHARED_LIBRARY)
	#define HalleyGame(T) HALLEY_EXPORT IHalleyEntryPoint* getHalleyEntry() { static HalleyEntryPoint<T> entry; return &entry; }
#endif
