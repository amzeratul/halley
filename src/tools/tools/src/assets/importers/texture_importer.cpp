#include "texture_importer.h"

#include "halley/bytes/compression.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file_formats/image.h"

using namespace Halley;

TextureImporter::TextureImporter(bool lz4hc)
	: lz4hc(lz4hc)
{
}

void TextureImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	// Get image
	Image image;
	Deserializer s(asset.inputFiles.at(0).data);
	s >> image;
	auto meta = asset.inputFiles.at(0).metadata;

	const bool useQOI = false;
	const bool useHLIF = true;

	if (useHLIF) {
		meta.set("compression", "hlif");
		collector.output(asset.assetId, AssetType::Texture, image.saveHLIFToBytes(asset.assetId, lz4hc), meta);
	} else if (useQOI && (image.getFormat() == Image::Format::RGB || image.getFormat() == Image::Format::RGBA || image.getFormat() == Image::Format::RGBAPremultiplied)) {
		meta.set("compression", "qoi");
		collector.output(asset.assetId, AssetType::Texture, image.saveQOIToBytes(), meta);
	} else {
		meta.set("compression", "png");
		collector.output(asset.assetId, AssetType::Texture, image.savePNGToBytes(), meta);
	}
}
