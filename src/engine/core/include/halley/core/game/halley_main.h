#pragma once

#include <halley/runner/game_loader.h>
#include <halley/core/game/core.h>
#include "halley/runner/main_loop.h"
#include "halley/runner/entry_point.h"

namespace Halley
{
	class Game;

	class HalleyMain {
	public:
		template <typename T>
		static int winMain()
		{
			StaticGameLoader<T> reloader;
			return runMain(reloader, getWin32Args());
		}

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

		static int runMain(GameLoader& loader, const Vector<std::string>& args);

		static Vector<std::string> getWin32Args();
	};
}

#ifdef _MSC_VER
	#define HALLEY_EXPORT extern "C" __declspec(dllexport)
#else
	#define HALLEY_EXPORT
#endif

// Macro to implement program
#if defined(HALLEY_EXECUTABLE)
	#if (defined(_WIN32) || defined(WINDOWS_STORE)) && !defined(__MINGW32__)
		#define HalleyGame(T) int __stdcall WinMain(void*, void*, char*, int) { return Halley::HalleyMain::winMain<T>(); }
	#else
		#define HalleyGame(T) int main(int argc, char* argv[]) { return Halley::HalleyMain::main<T>(argc, argv); }
	#endif
#elif defined(HALLEY_SHARED_LIBRARY)
	#define HalleyGame(T) HALLEY_EXPORT IHalleyEntryPoint* getHalleyEntry() { static HalleyEntryPoint<T> entry; return &entry; }
#endif
