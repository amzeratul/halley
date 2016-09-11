#include "font_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include <iostream>
#include "halley/maths/vector2.h"
#include "halley/maths/range.h"
#include "halley/tools/make_font/font_generator.h"
#include "halley/resources/metadata.h"

using namespace Halley;

std::vector<Path> FontImporter::import(const ImportAssetsDatabaseEntry& asset, Path dstDir, ProgressReporter reporter)
{
	std::cout << "Importing font " << asset.assetId << std::endl;

	Path src = getMainFile(asset);
	if (src == Path()) {
		return {};
	}

	FileSystem::createDir(dstDir);

	Vector2i imgSize(512, 512);
	float radius = 8;
	int supersample = 4;
	auto meta = getMetaData(asset);
	if (meta) {
		radius = meta->getFloat("radius", 8);
		supersample = meta->getInt("supersample", 4);
		imgSize.x = meta->getInt("width", 512);
		imgSize.y = meta->getInt("height", 512);
	}

	FontGenerator gen(false, reporter);
	auto result = gen.generateFont(asset.assetId, asset.srcDir / src, imgSize, radius, supersample, Range<int>(0, 255));
	return result.write(dstDir, true);
}
