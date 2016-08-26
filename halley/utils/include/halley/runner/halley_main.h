#pragma once

#include <halley/runner/game_loader.h>
#include <halley/text/halleystring.h>
#include <halley/support/console.h>
#include <halley/core/game/core.h>
#include "main_loop.h"
#include "entry_point.h"

#ifdef _WIN32
extern "C" {
	__declspec(dllimport) wchar_t* __stdcall CommandLineToArgvW(wchar_t*, int*);
	__declspec(dllimport) wchar_t* __stdcall GetCommandLineW();
	__declspec(dllimport) void* __stdcall LocalFree(void*);
}
#endif

namespace Halley
{
	class Game;

	class HalleyMain {
	public:
#ifdef _WIN32
		template <typename T>
		static int winMain()
		{
			auto cmd = GetCommandLineW();
			int argc;
			auto argv = CommandLineToArgvW(cmd, &argc);
			Vector<std::string> args;
			for (int i = 0; i < argc; i++) {
				args.push_back(String(reinterpret_cast<wchar_t**>(argv)[i]).cppStr());
			}
			LocalFree(argv);

			StaticGameLoader<T> reloader;
			return runMain(reloader, args);
		}
#endif

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
				std::cout << ConsoleColour(Console::RED) << "\n\nUnhandled exception: " << ConsoleColour(Console::DARK_RED) << e.what() << ConsoleColour() << std::endl;
				return 1;
			}
			catch (...) {
				std::cout << ConsoleColour(Console::RED) << "\n\nUnknown unhandled exception." << ConsoleColour() << std::endl;
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
	#if defined(_WIN32)
		#define HalleyGame(T) int __stdcall WinMain(void*, void*, char*, int) { return Halley::HalleyMain::winMain<T>(); }
	#else
		#define HalleyGame(T) int main(int argc, char* argv[]) { return Halley::HalleyMain::main<T>(argc, argv); }
	#endif
#elif defined(HALLEY_SHARED_LIBRARY)
	#define HalleyGame(T) HALLEY_EXPORT IHalleyEntryPoint* getHalleyEntry() { static HalleyEntryPoint<T> entry; return &entry; }
#endif
