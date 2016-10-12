#include "spritesheet_importer.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file/byte_serializer.h"

std::vector<Halley::Path> Halley::SpriteSheetImporter::import(const ImportingAsset& asset, Path dstDir, ProgressReporter reporter, AssetCollector collector)
{
	SpriteSheet sheet;
	Path dstFile = dstDir / asset.inputFiles[0].name.replaceExtension("");
	sheet.loadJson(gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));
	FileSystem::writeFile(dstFile, Serializer::toBytes(sheet));
	return { dstFile };
}
