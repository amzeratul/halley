#include "halley/tools/assets/asset_collector.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/metadata.h"

using namespace Halley;


AssetCollector::AssetCollector(const ImportingAsset& asset, const Path& dstDir, const std::vector<Path>& assetsSrc, ProgressReporter reporter)
	: asset(asset)
	, dstDir(dstDir)
	, assetsSrc(assetsSrc)
	, reporter(reporter)
{}

void AssetCollector::output(AssetType type, const Bytes& data, Maybe<Metadata> metadata)
{
	output(type, gsl::as_bytes(gsl::span<const Byte>(data)), metadata);
}

void AssetCollector::output(AssetType type, gsl::span<const gsl::byte> data, Maybe<Metadata> metadata)
{
	Path filePath = dstDir / toString(type) / asset.assetId;
	FileSystem::writeFile(filePath, data);

	AssetResource result;
	result.type = type;
	result.filepath = filePath.string();
	if (metadata) {
		result.metadata = metadata.get();
	}
	assets.emplace_back(result);
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

std::vector<AssetResource> AssetCollector::collectAssets()
{
	return std::move(assets);
}

std::vector<ImportingAsset> AssetCollector::collectAdditionalAssets()
{
	return std::move(additionalAssets);
}
