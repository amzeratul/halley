#pragma once

#include "halley/core/halley_core.h"

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
			return staticMain(std::make_unique<T>(), args);
		}

		static int staticMain(std::unique_ptr<Game> game, const StringArray& args);
		static int dynamicMain(String dllName, const StringArray& args);
	};
}

// Macro to implement program
#define HalleyGame(T) int main(int argc, char* argv[]) { return Halley::HalleyMain::main<T>(argc, argv); }
