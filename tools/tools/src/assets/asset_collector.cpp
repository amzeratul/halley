#include "halley/tools/assets/asset_collector.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/metadata.h"

using namespace Halley;


AssetCollector::AssetCollector(const Path& dstDir, const std::vector<Path>& assetsSrc, ProgressReporter reporter)
	: dstDir(dstDir)
	, assetsSrc(assetsSrc)
	, reporter(reporter)
{}

void AssetCollector::output(const Path& path, const Bytes& data, Maybe<Metadata> metadata)
{
	output(path, gsl::as_bytes(gsl::span<const Byte>(data)), metadata);
}

void AssetCollector::output(const Path& path, gsl::span<const gsl::byte> data, Maybe<Metadata> metadata)
{
	FileSystem::writeFile(dstDir / path, data);
	outFiles.emplace_back(path);

	if (metadata) {
		Path metaPath = path.replaceExtension(path.getExtension() + ".meta");
		FileSystem::writeFile(dstDir / metaPath, Serializer::toBytes(metadata.get()));
		outFiles.emplace_back(metaPath);
	}
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
