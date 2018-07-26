#include <iostream>
#include <halley/data_structures/vector.h>
#include <halley/tools/cli_tool.h>
#include "halley/core/game/environment.h"

int main(int argc, char** argv)
{
	Halley::CommandLineTools tools;
	Halley::Vector<std::string> names = tools.getToolNames();

	if (argc < 2) {
		std::cout << "Usage: halley-cmd [tool]" << std::endl;
		std::cout << "Available tools:" << std::endl;
		for (auto& n : names) {
			std::cout << "- " << n << std::endl;
		}
		return 1;
	} else {
		std::unique_ptr<Halley::CommandLineTool> tool;
		try {
			tool = tools.getTool(argv[1]);
		} catch (...) {
			std::cout << "Unknown tool: " << argv[1] << std::endl;
			return 1;
		}

		try {
			return tool->runRaw(argc, argv);
		} catch (std::exception& e) {
			std::cout << "Exception: " << e.what() << std::endl;
			return 2;
		}
	}
}
