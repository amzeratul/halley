#include "aseprite_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/file_formats/image.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

std::vector<Path> AsepriteImporter::import(const ImportingAsset& asset, Path dstDir, ProgressReporter reporter, AssetCollector collector)
{
	// Make temporary folder
	Path tmp = FileSystem::getTemporaryPath();
	FileSystem::createDir(tmp);
	Path srcPath = asset.inputFiles.at(0).name;
	Path tmpFilePath = tmp / srcPath;
	FileSystem::createParentDir(tmpFilePath);
	auto span = gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles[0].data));
	FileSystem::writeFile(tmpFilePath, span);

	// Run aseprite
	Path jsonPath = (tmpFilePath.parentPath() / "data.json");
	Path baseOutputPath = (tmpFilePath.parentPath() / srcPath.getStem());
	FileSystem::runCommand("aseprite -b " + tmpFilePath.getString() + " --list-tags --data " + jsonPath.getString() + " --save-as " + baseOutputPath.getString() + ".png");

	// Generate spritesheet
	// TODO

	// Generate animation
	// TODO

	Path spriteSheetPath;
	Path imagePath = srcPath;
	Path animationPath;

	// Image metafile
	Metadata meta;
	if (asset.metadata) {
		meta = *asset.metadata;
	}
	//meta.set("width", size.x);
	//meta.set("height", size.y);
	Path imageMetaPath = imagePath.replaceExtension(imagePath.getExtension() + ".meta");
	FileSystem::writeFile(dstDir / imageMetaPath, Serializer::toBytes(meta));

	// Remove temp
	FileSystem::remove(tmp);

	return { spriteSheetPath, animationPath, imagePath, imageMetaPath };
}
