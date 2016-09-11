#include "dynamic_loader.h"
#include <boost/filesystem.hpp>
#include <halley/runner/main_loop.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#ifdef _MSC_VER
#pragma comment(lib, "SDL2.lib")
#endif

using namespace Halley;

// Replacement for Halley::runMain
static int runMain(DynamicGameLoader& loader, Vector<std::string> args)
{
	std::unique_ptr<IMainLoopable> core;
	try {
		core = loader.createCore(args);

		loader.setCore(*core);
		MainLoop loop(*core, loader);
		loop.run();
		core.reset();
		
		std::cout << "Main loop terminated normally." << std::endl;
		return 0;
	} catch (std::exception& e) {
		if (core) {
			core->onTerminatedInError(e.what());
		} else {
			std::cout << "Error initializing core: " << e.what() << std::endl;
		}
		return 1;
	} catch (...) {
		if (core) {
			core->onTerminatedInError("");
		} else {
			std::cout << "Unknown error initializing core." << std::endl;
		}
		return 1;
	}
}

int main(int argc, char** argv) {
	SDL_SetMainReady();
	SDL_Init(SDL_INIT_NOPARACHUTE);

	Vector<std::string> args;
	for (int i = 0; i < argc; i++) {
		args.push_back(argv[i]);
	}

	using namespace boost::filesystem;
	path p(args[1]);

	DynamicGameLoader loader(p.string());

	return runMain(loader, args);
}
