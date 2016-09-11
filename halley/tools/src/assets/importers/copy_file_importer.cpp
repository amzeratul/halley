#include "copy_file_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/metadata.h"

using namespace Halley;

std::vector<Path> CopyFileImporter::import(const ImportAssetsDatabaseEntry& asset, Path dstDir, ProgressReporter reporter)
{
	auto srcDir = asset.srcDir;

	std::vector<Path> out;
	
	Path mainFile = getMainFile(asset);
	FileSystem::copyFile(srcDir / mainFile, dstDir / mainFile);
	out.push_back(mainFile);

	auto meta = getMetaData(asset);
	if (meta) {
		Path metaPath = mainFile;
		metaPath.replace_extension(metaPath.extension().string() + ".meta");
		FileSystem::writeFile(dstDir / metaPath, Serializer::toBytes(*meta));
		out.push_back(metaPath);
	}

	return out;
}
