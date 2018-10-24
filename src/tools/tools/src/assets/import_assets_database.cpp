#include "halley/tools/assets/import_assets_database.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/resource_data.h"
#include "halley/tools/file/filesystem.h"

constexpr static int currentAssetVersion = 50;

using namespace Halley;

void ImportAssetsDatabaseEntry::serialize(Serializer& s) const
{
	s << assetId;
	s << srcDir;
	s << inputFiles;
	s << additionalInputFiles;
	s << outputFiles;
	int t = int(assetType);
	s << t;
}

void ImportAssetsDatabaseEntry::deserialize(Deserializer& s)
{
	s >> assetId;
	s >> srcDir;
	s >> inputFiles;
	s >> additionalInputFiles;
	s >> outputFiles;
	int t;
	s >> t;
	assetType = ImportAssetType(t);
}

void ImportAssetsDatabase::AssetEntry::serialize(Serializer& s) const
{
	s << asset;
}

void ImportAssetsDatabase::AssetEntry::deserialize(Deserializer& s)
{
	s >> asset;
}

void ImportAssetsDatabase::InputFileEntry::serialize(Serializer& s) const
{
	const int nTimestamps = int(timestamp.size());
	s << nTimestamps;
	for (int i = 0; i < nTimestamps; ++i) {
		s << timestamp[i];
	}
	s << metadata;
}

void ImportAssetsDatabase::InputFileEntry::deserialize(Deserializer& s)
{
	int nTimestamps;
	s >> nTimestamps;
	for (int i = 0; i < nTimestamps; ++i) {
		if (i < int(timestamp.size())) {
			s >> timestamp[i];
		}
	}
	for (int i = nTimestamps; i < int(timestamp.size()); ++i) {
		timestamp[i] = 0;
	}
	s >> metadata;
}

ImportAssetsDatabase::ImportAssetsDatabase(Path directory, Path dbFile, Path assetsDbFile, std::vector<String> platforms)
	: platforms(std::move(platforms))
	, directory(directory)
	, dbFile(dbFile)
	, assetsDbFile(assetsDbFile)
{
	load();
}

void ImportAssetsDatabase::load()
{
	std::lock_guard<std::mutex> lock(mutex);
	auto data = FileSystem::readFile(dbFile);
	if (data.size() > 0) {
		auto s = Deserializer(data);
		deserialize(s);
	}
}

void ImportAssetsDatabase::save() const
{
	std::lock_guard<std::mutex> lock(mutex);
	FileSystem::writeFile(dbFile, Serializer::toBytes(*this));
	for (auto& platform: platforms) {
		// TODO: fix this
		auto assetDb = makeAssetDatabase(platform);
		FileSystem::writeFile(assetsDbFile, Serializer::toBytes(*assetDb));
	}
}

bool ImportAssetsDatabase::needToLoadInputMetadata(const Path& path, std::array<int64_t, 3> timestamps) const
{
	std::lock_guard<std::mutex> lock(mutex);

	// Is it an unknown file?
	auto pathStr = path.toString();
	auto iter = inputFiles.find(pathStr);
	if (iter == inputFiles.end()) {
		return true;
	}

	// Any of the timestamps changed?
	for (int i = 0; i < timestamps.size(); ++i) {
		if (iter->second.timestamp[i] != timestamps[i]) {
			return true;
		}
	}

	return false;
}

void ImportAssetsDatabase::setInputFileMetadata(const Path& path, std::array<int64_t, 3> timestamps, const Metadata& data)
{
	std::lock_guard<std::mutex> lock(mutex);

	auto pathStr = path.toString();
	auto& input = inputFiles[pathStr];
	input.timestamp = timestamps;
	input.metadata = data;
}

Maybe<Metadata> ImportAssetsDatabase::getMetadata(const Path& path) const
{
	std::lock_guard<std::mutex> lock(mutex);

	auto pathStr = path.toString();
	auto iter = inputFiles.find(pathStr);
	if (iter == inputFiles.end()) {
		return {};
	} else {
		return iter->second.metadata;
	}
}

bool ImportAssetsDatabase::needsImporting(const ImportAssetsDatabaseEntry& asset) const
{
	std::lock_guard<std::mutex> lock(mutex);
	
	// Check if it failed loading last time
	auto iter = assetsFailed.find(asset.assetId);
	bool failed = iter != assetsFailed.end();
	if (!failed) {
		// No failures, check if this was imported before
		iter = assetsImported.find(asset.assetId);
		if (iter == assetsImported.end()) {
			// Asset didn't even exist before
			return true;
		}
	}

	// At this point, iter points to the failed one if it failed, or the the old successful one if it didn't.
	auto& oldAsset = iter->second.asset;

	// Input directory changed?
	if (asset.srcDir != oldAsset.srcDir) {
		return true;
	}

	// Total count of input files changed?
	if (asset.inputFiles.size() != oldAsset.inputFiles.size()) {
		return true;
	}

	// Any of the input files changed?
	// Note: We don't have to check old files on new input, because the size matches and all entries matched.
	for (auto& i: asset.inputFiles) {
		auto result = std::find_if(oldAsset.inputFiles.begin(), oldAsset.inputFiles.end(), [&](const TimestampedPath& entry) { return entry.first == i.first; });
		if (result == oldAsset.inputFiles.end()) {
			// File wasn't there before
			return true;
		} else if (result->second != i.second) {
			// Timestamp changed
			return true;
		}
	}

	// Any of the additional input files changed?
	for (auto& i: oldAsset.additionalInputFiles) {
		if (!FileSystem::exists(i.first)) {
			// File removed
			return true;
		} else if  (FileSystem::getLastWriteTime(i.first) != i.second) {
			// Timestamp changed
			return true;
		}		
	}

	// Have any of the output files gone missing?
	if (!failed) {
		for (auto& o: oldAsset.outputFiles) {
			for (auto& version: o.platformVersions) {
				if (!FileSystem::exists(directory / version.second.filepath)) {
					return true;
				}
			}
		}
	}

	return false;
}

void ImportAssetsDatabase::markAsImported(const ImportAssetsDatabaseEntry& asset)
{
	AssetEntry entry;
	entry.asset = asset;
	entry.present = true;

	std::lock_guard<std::mutex> lock(mutex);
	assetsImported[asset.assetId] = entry;
	
	auto failIter = assetsFailed.find(asset.assetId);
	if (failIter != assetsFailed.end()) {
		assetsFailed.erase(failIter);
	}
}

void ImportAssetsDatabase::markDeleted(const ImportAssetsDatabaseEntry& asset)
{
	std::lock_guard<std::mutex> lock(mutex);
	assetsImported.erase(asset.assetId);
}

void ImportAssetsDatabase::markFailed(const ImportAssetsDatabaseEntry& asset)
{
	AssetEntry entry;
	entry.asset = asset;
	entry.present = true;

	std::lock_guard<std::mutex> lock(mutex);
	assetsFailed[asset.assetId] = entry;
}

void ImportAssetsDatabase::markAssetsAsStillPresent(const std::map<String, ImportAssetsDatabaseEntry>& assets)
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& e: assetsImported) {
		e.second.present = assets.find(e.first) != assets.end();
	}
}

std::vector<ImportAssetsDatabaseEntry> ImportAssetsDatabase::getAllMissing() const
{
	std::lock_guard<std::mutex> lock(mutex);
	std::vector<ImportAssetsDatabaseEntry> result;
	for (auto& e : assetsImported) {
		if (!e.second.present) {
			result.push_back(e.second.asset);
		}
	}
	return result;
}

std::vector<AssetResource> ImportAssetsDatabase::getOutFiles(String assetId) const
{
	auto iter = assetsImported.find(assetId);
	if (iter != assetsImported.end()) {
		return iter->second.asset.outputFiles;
	} else {
		return {};
	}
}

void ImportAssetsDatabase::serialize(Serializer& s) const
{
	int version = currentAssetVersion;
	s << version;
	s << platforms;
	s << assetsImported;
	s << inputFiles;
}

void ImportAssetsDatabase::deserialize(Deserializer& s)
{
	int version;
	s >> version;
	if (version == currentAssetVersion) {
		std::vector<String> platformsRead;
		s >> platformsRead;
		if (platformsRead == platforms) {
			s >> assetsImported;
			s >> inputFiles;
		}
	}
}

std::unique_ptr<AssetDatabase> ImportAssetsDatabase::makeAssetDatabase(const String& platform) const
{
	auto result = std::make_unique<AssetDatabase>();
	for (auto& a: assetsImported) {
		auto& asset = a.second.asset;
		for (auto& o: asset.outputFiles) {
			auto iter = o.platformVersions.find(platform);
			const AssetResource::PlatformVersion* version = nullptr;
			if (iter != o.platformVersions.end()) {
				version = &iter->second;
			} else {
				iter = o.platformVersions.find("pc");
				if (iter != o.platformVersions.end()) {
					version = &iter->second;
				}
			}

			if (version) {
				result->addAsset(o.name, o.type, AssetDatabase::Entry(version->filepath, version->metadata));
			}
		}
	}
	return result;
}
