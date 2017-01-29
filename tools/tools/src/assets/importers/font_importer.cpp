#include "font_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include <iostream>
#include "halley/maths/vector2.h"
#include "halley/maths/range.h"
#include "halley/tools/make_font/font_generator.h"
#include "halley/file_formats/image.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

void FontImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	Vector2i imgSize(512, 512);
	float radius = 8;
	int supersample = 4;
	auto& meta = asset.metadata;
	if (meta) {
		radius = meta->getFloat("radius", 8);
		supersample = meta->getInt("supersample", 4);
		imgSize.x = meta->getInt("width", 512);
		imgSize.y = meta->getInt("height", 512);
	}

	auto data = gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles[0].data));

	FontGenerator gen(false, [&] (float progress, const String& label)
	{
		return collector.reportProgress(progress, label);
	});
	auto result = gen.generateFont(asset.assetId, data, imgSize, radius, supersample, Range<int>(0, 255));
	if (!result.success) {
		throw Exception("Failed to generate font: " + asset.assetId);
	}
	
	Path fileName = Path(asset.assetId).replaceExtension(".font");
	Path pngPath = fileName.replaceExtension(".png");
	collector.output(fileName, result.fontData);

	auto imgData = result.image->savePNGToBytes();

	ImportingAsset image;
	image.assetId = asset.assetId + "-image";
	image.assetType = AssetType::Image;
	image.metadata = std::move(result.imageMeta);
	image.inputFiles.emplace_back(ImportingAssetFile(pngPath, std::move(imgData)));
	collector.addAdditionalAsset(std::move(image));
}
