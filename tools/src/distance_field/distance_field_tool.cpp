#include "distance_field_tool.h"
#include "distance_field_generator.h"

using namespace Halley;

int DistanceFieldTool::run(std::vector<std::string> args)
{
	if (args.size() != 4) {
		std::cout << "Usage: halley-cmd distField srcFile dstFile WxH radius" << std::endl;
		return 1;
	}

	// Properties
	auto res = String(args[2]).split('x');
	Vector2i size(res[0].toInteger(), res[1].toInteger());
	float radius = String(args[3]).toFloat();

	// Load image
	auto data = ResourceDataStatic::loadFromFileSystem(args[0]);
	auto inputImg = std::make_unique<Image>(args[0], reinterpret_cast<const Byte*>(data->getData()), data->getSize(), false);

	// Process image
	auto result = DistanceFieldGenerator::generate(*inputImg, size, radius);
	inputImg.reset();

	// Output image
	result->savePNG(args[1]);
	
	return 0;
}
