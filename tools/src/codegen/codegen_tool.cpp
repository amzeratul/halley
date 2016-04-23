#include "codegen_tool.h"
#include "codegen.h"
#include <iostream>

using namespace Halley;

int CodegenTool::run(std::vector<std::string> args)
{
	if (args.size() == 2) {
		Codegen::run(args[0], args[1]);
		return 0;
	} else {
		std::cout << "Usage: halley codegen srcDir dstDir" << std::endl;
		return 1;
	}
}
