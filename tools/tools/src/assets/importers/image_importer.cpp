#include "image_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/file_formats/image.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

std::vector<Path> ImageImporter::import(const ImportingAsset& asset, Path dstDir, ProgressReporter reporter, AssetCollector collector)
{
	// Load image
	Path mainFile = asset.inputFiles.at(0).name;
	auto span = gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles[0].data));
	Vector2i size = Image::getImageSize(mainFile.string(), span);
	FileSystem::writeFile(dstDir / mainFile, span);

	// Prepare metafile
	Metadata meta;
	if (asset.metadata) {
		meta = *asset.metadata;
	}

	meta.set("width", size.x);
	meta.set("height", size.y);

	Path metaPath = mainFile.replaceExtension(mainFile.getExtension() + ".meta");
	FileSystem::writeFile(dstDir / metaPath, Serializer::toBytes(meta));

	return { mainFile, metaPath };
}
