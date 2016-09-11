#include "image_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/file_formats/image.h"

using namespace Halley;

std::vector<Path> ImageImporter::import(const ImportAssetsDatabaseEntry& asset, Path dstDir, ProgressReporter reporter)
{
	auto srcDir = asset.srcDir;

	// Load image
	Path mainFile = getMainFile(asset);
	auto loader = ResourceDataStatic::loadFromFileSystem(srcDir / mainFile);
	//Image img("", loader->getSpan(), false);
	//Vector2i size = img.getSize();
	Vector2i size = Image::getImageSize(mainFile.string(), loader->getSpan());
	FileSystem::writeFile(dstDir / mainFile, loader->getSpan());

	// Prepare metafile
	auto meta = getMetaData(asset);
	if (!meta) {
		meta = std::make_unique<Metadata>();
	}

	meta->set("width", size.x);
	meta->set("height", size.y);

	Path metaPath = mainFile;
	metaPath.replace_extension(metaPath.extension().string() + ".meta");
	FileSystem::writeFile(dstDir / metaPath, Serializer::toBytes(*meta));

	return { mainFile, metaPath };
}
