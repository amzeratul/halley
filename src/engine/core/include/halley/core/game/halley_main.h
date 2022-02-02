#pragma once

#include <halley/core/entry/game_loader.h>
#include <halley/core/game/core.h>
#include "halley/core/game/main_loop.h"
#include "halley/core/entry/entry_point.h"
#include "halley/entity/create_functions.h"
#include "halley/entity/registry.h"

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
			StaticGameLoader<T> reloader;
			return runMain(reloader, getArgs(argc, argv));
		}

		static int runMain(GameLoader& loader, const Vector<std::string>& args);

		static Vector<std::string> getWin32Args();
		static Vector<std::string> getArgs(int argc, char* argv[]);
	};
	
	template <typename T> constexpr static void InitEntities()
	{
		CreateEntityFunctions::getCreateComponent() = createComponent;
		CreateEntityFunctions::getCreateSystem() = createSystem;
		CreateEntityFunctions::getCreateMessage() = createMessage;
	}
}

#ifdef _MSC_VER
	#define HALLEY_EXPORT extern "C" __declspec(dllexport)
#else
	#define HALLEY_EXPORT
#endif

#if defined(HALLEY_EXECUTABLE)
	#if defined(_WIN32) || defined(WINDOWS_STORE)
		#define HalleyGame(T) int __stdcall WinMain(void*, void*, char*, int) { Halley::InitEntities<T>(); return Halley::HalleyMain::winMain<T>(); }
	#else
		#define HalleyGame(T) int main(int argc, char* argv[]) { Halley::InitEntities<T>(); return Halley::HalleyMain::main<T>(argc, argv); }
	#endif
#elif defined(HALLEY_SHARED_LIBRARY)
	#define HalleyGame(T) HALLEY_EXPORT IHalleyEntryPoint* getHalleyEntry() { Halley::InitEntities<T>(); static HalleyEntryPoint<T> entry; return &entry; }
#elif defined(HALLEY_STATIC_LIBRARY)
	#define HalleyGame(T) IHalleyEntryPoint* getHalleyEntryStatic() { Halley::InitEntities<T>(); static HalleyEntryPoint<T> entry; return &entry; }
#endif
