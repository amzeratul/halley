#include "distance_field_tool.h"
#include "distance_field_generator.h"

using namespace Halley;

int DistanceFieldTool::run(std::vector<std::string> args)
{
	if (args.size() != 3) {
		std::cout << "Usage: halley-cmd distField srcFile dstFile WxH" << std::endl;
		return 1;
	}

	// Properties
	Vector2i size(256, 128); // TODO
	float radius = 5; // TODO

	// Load image
	auto data = ResourceDataStatic::loadFromFileSystem(args[0]);
	auto inputImg = std::make_unique<Image>(args[0], reinterpret_cast<const Byte*>(data->getData()), data->getSize(), false);

	// Process image
	auto result = DistanceFieldGenerator::generate(*inputImg, size, radius);
	inputImg.reset();

	// Output image
	result->savePNG(args[1]);
	
	system(args[1].c_str());
	
	return 0;
}
