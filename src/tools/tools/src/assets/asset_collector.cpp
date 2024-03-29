#include "halley/tools/assets/asset_collector.h"
#include "halley/tools/file/filesystem.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/support/logger.h"
#include "halley/bytes/compression.h"
#include "halley/utils/algorithm.h"

using namespace Halley;


AssetCollector::AssetCollector(const ImportingAsset& asset, const Path& dstDir, const Vector<Path>& assetsSrc, ProgressReporter reporter)
	: asset(asset)
	, dstDir(dstDir)
	, assetsSrc(assetsSrc)
	, reporter(std::move(reporter))
{}

void AssetCollector::output(const String& name, AssetType type, const Bytes& data, std::optional<Metadata> metadata, const String& platform, const Path& primaryInputFile)
{
	const String id = name.replaceAll("_", "__").replaceAll("/", "_-_").replaceAll(":", "_c_");
	Path filePath = Path(toString(type)) / id;
	Path fullPath = Path(platform) / filePath;

	if (metadata && metadata->getString("asset_compression", "") == "lz4") {
		Compression::LZ4Options options;
		options.mode = Compression::LZ4Mode::HC;
		outFiles.emplace_back(fullPath, Compression::lz4CompressFile(data.byte_span(), {}, options));
	} else if (metadata && metadata->getString("asset_compression", "") == "deflate") {
		auto newData = Compression::compress(data);
		outFiles.emplace_back(fullPath, newData);
	} else {
		outFiles.emplace_back(fullPath, data);
	}
	
	// Store information about this asset version
	AssetResource::PlatformVersion version;
	version.filepath = fullPath.string();
	if (metadata) {
		version.metadata = metadata.value();
	}
	getAsset(name, type, primaryInputFile).platformVersions[platform] = std::move(version);
}

AssetResource& AssetCollector::getAsset(const String& name, AssetType type, const Path& primaryInputFile)
{
	AssetResource* result = nullptr;
	for (auto& a: assets) {
		if (a.name == name && a.type == type) {
			result = &a;
		}
	}
	if (!result) {
		assets.emplace_back();
		result = &assets.back();
		result->name = name;
		result->type = type;
		result->primaryInputFile = primaryInputFile;
	}
	return *result;
}

void AssetCollector::output(const String& name, AssetType type, const Path& path, gsl::span<const gsl::byte> data)
{
	Bytes result;
	result.resize(data.size());
	memcpy(result.data(), data.data(), data.size());
	outFiles.emplace_back(path, std::move(result));

	AssetResource::PlatformVersion version;
	version.filepath = path.getString();
	getAsset(name, type).platformVersions[""] = std::move(version);
}

void AssetCollector::output(const String& name, AssetType type, const Path& path)
{
	outFiles.emplace_back(path, std::nullopt);

	AssetResource::PlatformVersion version;
	version.filepath = path.getString();
	getAsset(name, type).platformVersions[""] = std::move(version);
}

void AssetCollector::addAdditionalAsset(ImportingAsset&& additionalAsset)
{
	additionalAssets.emplace_back(std::move(additionalAsset));
}

bool AssetCollector::reportProgress(float progress, const String& label)
{
	return reporter ? reporter(progress, label) : true;
}

const Path& AssetCollector::getDestinationDirectory()
{
	return dstDir;
}

Bytes AssetCollector::readAdditionalFile(const Path& filePath)
{
	for (const auto& path: assetsSrc) {
		Path f = path / filePath;
		if (FileSystem::exists(f)) {
			if (!std_ex::contains_if(additionalInputs, [&] (const auto& e) { return e.first == f; })) {
				additionalInputs.push_back(TimestampedPath(f, FileSystem::getLastWriteTime(f)));			
			}
			return FileSystem::readFile(f);
		}
	}
	throw Exception("Unable to find asset dependency: \"" + filePath.getString() + "\"", HalleyExceptions::Tools);
}

const Vector<AssetResource>& AssetCollector::getAssets() const
{
	return assets;
}

Vector<ImportingAsset> AssetCollector::collectAdditionalAssets()
{
	return std::move(additionalAssets);
}

Vector<std::pair<Path, std::optional<Bytes>>> AssetCollector::collectOutFiles()
{
	return std::move(outFiles);
}

const Vector<TimestampedPath>& AssetCollector::getAdditionalInputs() const
{
	return additionalInputs;
}
