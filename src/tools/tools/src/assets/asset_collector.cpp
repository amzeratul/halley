#include "halley/tools/assets/asset_collector.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/support/logger.h"
#include "halley/file/compression.h"

using namespace Halley;


AssetCollector::AssetCollector(const ImportingAsset& asset, const Path& dstDir, const std::vector<Path>& assetsSrc, ProgressReporter reporter)
	: asset(asset)
	, dstDir(dstDir)
	, assetsSrc(assetsSrc)
	, reporter(reporter)
{}

void AssetCollector::output(const String& name, AssetType type, const Bytes& data, Maybe<Metadata> metadata, const String& platform)
{
	const String id = name.replaceAll("_", "__").replaceAll("/", "_-_").replaceAll(":", "_c_");
	Path filePath = Path(toString(type)) / id;
	Path fullPath = Path(platform) / filePath;

	if (metadata && metadata->getString("asset_compression", "") == "deflate") {
		auto newData = Compression::compress(data);
		outFiles.emplace_back(fullPath, newData);
	} else {
		outFiles.emplace_back(fullPath, data);
	}

	AssetResource result;
	result.name = name;
	result.type = type;
	result.filepath = fullPath.string();
	if (metadata) {
		result.metadata = metadata.get();
	}
	assets.emplace_back(result);
}

void AssetCollector::addAdditionalAsset(ImportingAsset&& additionalAsset)
{
	additionalAssets.emplace_back(std::move(additionalAsset));
}

bool AssetCollector::reportProgress(float progress, const String& label)
{
	return reporter(progress, label);
}

const Path& AssetCollector::getDestinationDirectory()
{
	return dstDir;
}

Bytes AssetCollector::readAdditionalFile(const Path& filePath)
{
	for (auto path : assetsSrc) {
		Path f = path / filePath;
		if (FileSystem::exists(f)) {
			additionalInputs.push_back(TimestampedPath(f, FileSystem::getLastWriteTime(f)));
			return FileSystem::readFile(f);
		}
	}
	throw Exception("Unable to find asset dependency: \"" + filePath.getString() + "\"", HalleyExceptions::Tools);
}

const std::vector<AssetResource>& AssetCollector::getAssets() const
{
	return assets;
}

std::vector<ImportingAsset> AssetCollector::collectAdditionalAssets()
{
	return std::move(additionalAssets);
}

std::vector<std::pair<Path, Bytes>> AssetCollector::collectOutFiles()
{
	return std::move(outFiles);
}

const std::vector<TimestampedPath>& AssetCollector::getAdditionalInputs() const
{
	return additionalInputs;
}
