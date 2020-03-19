#include "halley/tools/runner/runner_tool.h"
#include "halley/tools/runner/dynamic_loader.h"
#include <boost/filesystem.hpp>
#include <halley/runner/main_loop.h>

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

int RunnerTool::runRaw(int argc, char* argv[])
{
	if (argc < 3) {
		std::cout << "Usage: halley-cmd run <dllname> ..." << std::endl;
		return 1;
	}
	
	// Skip the first two arguments (halley-cmd path and "run")
	std::vector<std::string> args;
	args.reserve(argc - 2);
	for (int i = 2; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}
	
	std::cout << "Running " << args.at(0) << "..." << std::endl;

	DynamicGameLoader loader(args.at(0));
	return runMain(loader, args);
}
