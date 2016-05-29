#include <iostream>
#include <vector>
#include <halley/tools/cli_tool.h>

int main(int argc, char** argv)
{
	std::vector<std::string> names = Halley::CommandLineTool::getToolNames();

	if (argc < 2) {
		std::cout << "Usage: halley [tool]" << std::endl;
		std::cout << "Available tools:" << std::endl;
		for (auto& n : names) {
			std::cout << "- " << n << std::endl;
		}
		return 1;
	} else {
		std::unique_ptr<Halley::CommandLineTool> tool;
		try {
			tool = Halley::CommandLineTool::getTool(argv[1]);
		} catch (...) {
			std::cout << "Unknown tool: " << argv[1] << std::endl;
			return 1;
		}

		try {
			return tool->runRaw(argc - 2, argv + 2);
		} catch (std::exception& e) {
			std::cout << "Exception: " << e.what() << std::endl;
			return 2;
		}
	}
}
