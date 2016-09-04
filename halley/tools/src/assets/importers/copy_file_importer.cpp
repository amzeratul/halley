#include "copy_file_importer.h"
#include "halley/tools/assets/import_assets_database.h"

using namespace Halley;

std::vector<Path> CopyFileImporter::import(const ImportAssetsDatabaseEntry& asset, Path dstDir)
{
	auto srcDir = asset.srcDir;

	std::vector<Path> out;
	for (auto& i : asset.inputFiles) {
		FileSystem::copyFile(srcDir / i.first, dstDir / i.first);
		out.emplace_back(dstDir / i.first);
	}

	return out;
}
