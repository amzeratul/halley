#include <halley/text/halleystring.h>
#include <halley/resources/resource_data.h>
#include <halley/file_formats/image.h>
#include "halley/tools/distance_field/distance_field_tool.h"
#include "halley/tools/distance_field/distance_field_generator.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

int DistanceFieldTool::run(Vector<std::string> args)
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
	auto inputImg = std::make_unique<Image>(*data);

	// Process image
	auto result = DistanceFieldGenerator::generate(*inputImg, size, radius);
	inputImg.reset();

	// Output image
	FileSystem::writeFile(Path(args[1]), result->savePNGToBytes());
	
	return 0;
}
