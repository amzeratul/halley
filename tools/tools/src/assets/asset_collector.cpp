#include "halley/tools/assets/asset_collector.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;


AssetCollector::AssetCollector(const Path& dstDir, const std::vector<Path>& assetsSrc, ProgressReporter reporter)
	: dstDir(dstDir)
	, assetsSrc(assetsSrc)
	, reporter(reporter)
{}

void AssetCollector::output(const Path& path, const Bytes& data)
{
	output(path, gsl::as_bytes(gsl::span<const Byte>(data)));
}

void AssetCollector::output(const Path& path, Bytes&& data)
{
	output(path, gsl::as_bytes(gsl::span<const Byte>(data)));
}

void AssetCollector::output(const Path& path, gsl::span<const gsl::byte> data)
{
	FileSystem::writeFile(dstDir / path, data);
	outFiles.emplace_back(path);
}

void AssetCollector::addAdditionalAsset(ImportingAsset&& asset)
{
	additionalAssets.emplace_back(std::move(asset));
}

bool AssetCollector::reportProgress(float progress, const String& label)
{
	return reporter(progress, label);
}

const Path& AssetCollector::getDestinationDirectory()
{
	return dstDir;
}

Bytes AssetCollector::readAdditionalFile(const Path& filePath) const
{
	for (auto path : assetsSrc) {
		Path f = path / filePath;
		if (FileSystem::exists(f)) {
			return FileSystem::readFile(f);
		}
	}
	throw Exception("Unable to find asset dependency: \"" + filePath.getString() + "\"");
}

const std::vector<Path>& AssetCollector::getOutFiles() const
{
	return outFiles;
}

std::vector<ImportingAsset> AssetCollector::collectAdditionalAssets()
{
	return std::move(additionalAssets);
}
