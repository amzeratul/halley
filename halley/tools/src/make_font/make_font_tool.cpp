#include "halley/tools/make_font/make_font_tool.h"
#include "halley/tools/make_font/font_generator.h"

using namespace Halley;

int MakeFontTool::run(Vector<std::string> args)
{
	if (args.size() != 5) {
		std::cout << "Usage: halley-cmd makeFont srcFont resultName WxH radius supersample" << std::endl;
		return 1;
	}

	Path source = args[0];
	Path target = args[1];
	auto res = String(args[2]).split('x');
	Vector2i size(res[0].toInteger(), res[1].toInteger());
	float radius = String(args[3]).toFloat();
	int superSample = String(args[4]).toInteger();

	FontGenerator generator(true);
	generator.generateFont(source, target, size, radius, superSample, Range<int>(0, 255));

	return 0;
}
