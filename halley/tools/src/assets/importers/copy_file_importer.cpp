#include "copy_file_importer.h"
#include "halley/tools/assets/import_assets_database.h"

using namespace Halley;

std::vector<Path> CopyFileImporter::import(const ImportAssetsDatabaseEntry& asset, Path dstDir, ProgressReporter reporter)
{
	auto srcDir = asset.srcDir;

	std::vector<Path> out;
	int n = 0;
	for (auto& i : asset.inputFiles) {
		if (!reporter(float(n) / asset.inputFiles.size(), "")) {
			return {};
		}
		FileSystem::copyFile(srcDir / i.first, dstDir / i.first);
		out.emplace_back(dstDir / i.first);
		++n;
	}

	return out;
}
