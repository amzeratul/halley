#include <iostream>
#include <vector>
#include "../../tools/src/tool/cli_tool.h"

int main(int argc, char** argv)
{
	std::vector<std::string> names = Halley::CommandLineTool::getToolNames();

	if (argc < 2) {
		std::cout << "Usage: halley [tool]" << std::endl;
		std::cout << "Available tools:" << std::endl;
		for (auto& n : names) {
			std::cout << "- " << n << std::endl;
		}
	} else {
		try {
			auto tool = Halley::CommandLineTool::getTool(argv[1]);
			tool->runRaw(argc - 2, argv + 2);
		} catch (...) {
			std::cout << "Unknown tool: " << argv[1] << std::endl;
		}
	}
}
