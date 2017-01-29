#include "spritesheet_importer.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file/byte_serializer.h"

using namespace Halley;

void SpriteSheetImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	SpriteSheet sheet;
	Path dstFile = asset.inputFiles[0].name.replaceExtension("");
	sheet.loadJson(gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));
	collector.output(dstFile, Serializer::toBytes(sheet));
}
