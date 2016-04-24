#include "distance_field_tool.h"

using namespace Halley;

int DistanceFieldTool::run(std::vector<std::string> args)
{
	if (args.size() != 3) {
		std::cout << "Usage: halley-cmd distField srcFile dstFile WxH" << std::endl;
		return 1;
	}

	return 0;
}
