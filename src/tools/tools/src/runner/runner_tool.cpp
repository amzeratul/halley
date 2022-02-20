#include "halley/tools/runner/runner_tool.h"
#include "halley/tools/runner/dynamic_loader.h"
#include "halley/core/game/halley_main.h"

using namespace Halley;

int RunnerTool::runRaw(int argc, char* argv[])
{
	if (argc < 3) {
		std::cout << "Usage: halley-cmd run <dllname> ..." << std::endl;
		return 1;
	}
	
	// Skip the first two arguments (halley-cmd path and "run")
	Vector<std::string> args;
	args.reserve(argc - 2);
	for (int i = 2; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}
	
	std::cout << "Running from DLL \"" << args.at(0) << "\"..." << std::endl;

	DynamicGameLoader loader(args.at(0));
	return HalleyMain::runMain(loader, args);
}
