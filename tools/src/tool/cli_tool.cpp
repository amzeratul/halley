#include "cli_tool.h"
#include "../codegen/codegen_tool.h"
#include <exception>

using namespace Halley;

std::vector<std::string> CommandLineTool::getToolNames()
{
	return {"codegen"};
}

std::unique_ptr<CommandLineTool> CommandLineTool::getTool(std::string name)
{
	if (name == "codegen") {
		return std::make_unique<CodegenTool>();
	} else {
		throw std::exception("Unknown tool name");
	}
}

void CommandLineTool::runRaw(int argc, char** argv)
{
	std::vector<std::string> args(argc);
	for (int i = 0; i < argc; i++) {
		args[i] = argv[i];
	}
	run(args);
}
