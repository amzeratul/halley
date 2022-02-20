#include "font_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/maths/vector2.h"
#include "halley/maths/range.h"
#include "halley/tools/make_font/font_generator.h"
#include "halley/file_formats/image.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/graphics/text/font.h"

using namespace Halley;

void FontImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	const auto& meta = asset.inputFiles.at(0).metadata;
	const float radius = meta.getFloat("radius", 8);
	const int supersample = meta.getInt("supersample", 4);
	Vector2i imgSize;
	imgSize.x = meta.getInt("width", 512);
	imgSize.y = meta.getInt("height", 512);
	const float fontSize = meta.getFloat("fontSize", 0);
	const float replacementScale = meta.getFloat("replacementScale", 1.0f);

	auto data = gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles[0].data));

	FontGenerator gen(false, [&] (float progress, const String& label)
	{
		return collector.reportProgress(progress, label);
	});

	FontGenerator::FontSizeInfo sizeInfo;
	if (fontSize != 0) {
		sizeInfo.fontSize = fontSize;
	} else {
		sizeInfo.imageSize = imgSize;
	}
	sizeInfo.replacementScale = replacementScale;

	auto range = Range<int>(meta.getInt("rangeStart", 0), meta.getInt("rangeEnd", 255));
	std::set<int> characterSet;
	for (int i = range.start; i <= range.end; ++i) {
		characterSet.insert(i);
	}
	auto extraChars = meta.getString("extraCharacters", "");
	for (int c: extraChars.getUTF32()) {
		characterSet.insert(c);
	}
	Vector<int> characters;
	characters.reserve(characterSet.size());
	for (auto& c: characterSet) {
		characters.push_back(c);
	}

	auto result = gen.generateFont(meta, data, sizeInfo, radius, supersample, characters);
	if (!result.success) {
		return;
	}

	auto fontName = result.font->getName();

	collector.output(fontName, AssetType::Font, Serializer::toBytes(*result.font));

	if (meta.hasKey("filtering")) {
		result.imageMeta->set("filtering", meta.getBool("filtering"));
	}

	ImportingAsset image;
	image.assetId = "fontTex/" + fontName;
	image.assetType = ImportAssetType::Image;
	image.inputFiles.emplace_back(ImportingAssetFile(fontName, Serializer::toBytes(*result.image), *result.imageMeta));
	collector.addAdditionalAsset(std::move(image));
}
