#include "make_font_tool.h"

using namespace Halley;

int MakeFontTool::run(std::vector<std::string> args)
{
	if (args.size() != 3) {
		std::cout << "Usage: halley-cmd makeFont fontFile dstFile WxH" << std::endl;
		return 1;
	}

	return 0;
}
