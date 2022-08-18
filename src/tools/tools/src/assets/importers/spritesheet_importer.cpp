#include "spritesheet_importer.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/tools/file/filesystem.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/file_formats/yaml_convert.h"

using namespace Halley;

void SpriteSheetImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	SpriteSheet sheet;
	sheet.load(YAMLConvert::parseConfig(asset.inputFiles.at(0).data).getRoot());

	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("asset_compression", "deflate");
	
	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::SpriteSheet, Serializer::toBytes(sheet), meta);
}
