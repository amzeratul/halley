#include <utility>
#include "halley/tools/assets/import_assets_database.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/resource_data.h"
#include "halley/tools/file/filesystem.h"

constexpr static int currentAssetVersion = 97;

using namespace Halley;

AssetPath::AssetPath()
{}

AssetPath::AssetPath(TimestampedPath path)
	: path(std::move(path))
{}

AssetPath::AssetPath(TimestampedPath path, Path dataPath)
	: path(std::move(path))
	, dataPath(std::move(dataPath))
{}

const Path& AssetPath::getPath() const
{
	return path.first;
}

const Path& AssetPath::getDataPath() const
{
	return dataPath.isEmpty() ? path.first : dataPath;
}

int64_t AssetPath::getTimestamp() const
{
	return path.second;
}

void AssetPath::serialize(Serializer& s) const
{
	s << path;
	s << dataPath;
}

void AssetPath::deserialize(Deserializer& s)
{
	s >> path;
	s >> dataPath;
}

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

int64_t ImportAssetsDatabaseEntry::getLatestTimestamp() const
{
	int64_t t = 0;
	for (auto& input: inputFiles) {
		t = std::max(t, input.getTimestamp());
	}
	for (auto& additional: additionalInputFiles) {
		t = std::max(t, additional.second);
	}
	return t;
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
	s << basePath;
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
	s >> basePath;
}

ImportAssetsDatabase::ImportAssetsDatabase(Path directory, Path dbFile, Path assetsDbFile, Vector<String> platforms)
	: platforms(std::move(platforms))
	, directory(std::move(directory))
	, dbFile(std::move(dbFile))
	, assetsDbFile(std::move(assetsDbFile))
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

	const auto pcAssetDatabase = makeAssetDatabase("pc");
	FileSystem::writeFile(assetsDbFile, Serializer::toBytes(*pcAssetDatabase));
}

bool ImportAssetsDatabase::needToLoadInputMetadata(const Path& path, std::array<int64_t, 3> timestamps) const
{
	std::lock_guard<std::mutex> lock(mutex);

	// Is it an unknown file?
	const auto iter = inputFiles.find(path.toString());
	if (iter == inputFiles.end()) {
		return true;
	}

	// Any of the timestamps changed?
	for (size_t i = 0; i < timestamps.size(); ++i) {
		if (iter->second.timestamp[i] != timestamps[i]) {
			return true;
		}
	}

	return false;
}

void ImportAssetsDatabase::setInputFileMetadata(const Path& path, std::array<int64_t, 3> timestamps, const Metadata& data, Path basePath)
{
	std::lock_guard<std::mutex> lock(mutex);

	auto& input = inputFiles[path.toString()];
	input.timestamp = timestamps;
	input.metadata = data;
	input.basePath = std::move(basePath);
	input.missing = false;
}

void ImportAssetsDatabase::markInputPresent(const Path& path)
{
	std::lock_guard<std::mutex> lock(mutex);
	auto& input = inputFiles[path.toString()];
	input.missing = false;
}

void ImportAssetsDatabase::markAllInputFilesAsMissing()
{
	std::lock_guard<std::mutex> lock(mutex);
	
	for (auto& i: inputFiles) {
		i.second.missing = true;
	}
}

bool ImportAssetsDatabase::purgeMissingInputs()
{
	std::lock_guard<std::mutex> lock(mutex);

	bool modified = false;
	for (auto iter = inputFiles.begin(); iter != inputFiles.end();) {
		if (iter->second.missing) {
			modified = true;
			iter = inputFiles.erase(iter);
		} else {
			++iter;
		}
	}
	
	return modified;
}

std::optional<Metadata> ImportAssetsDatabase::getMetadata(const Path& path) const
{
	std::lock_guard<std::mutex> lock(mutex);

	const auto pathStr = path.toString();
	const auto iter = inputFiles.find(pathStr);
	if (iter == inputFiles.end()) {
		return {};
	} else {
		return iter->second.metadata;
	}
}

std::optional<Metadata> ImportAssetsDatabase::getMetadata(AssetType type, const String& assetId) const
{
	std::lock_guard<std::mutex> lock(mutex);

	if (const auto * entry = findEntry(type, assetId); entry) {
		const auto& asset = entry->asset;
		for (auto& o: asset.outputFiles) {
			if (o.type == type && o.name == assetId) {
				const auto inputFile = o.primaryInputFile.isEmpty() ? asset.inputFiles.at(0).getPath() : o.primaryInputFile;

				const auto iter = inputFiles.find(inputFile.toString());
				if (iter == inputFiles.end()) {
					return {};
				}
				return iter->second.metadata;
			}
		}
	}

	return {};
}

Path ImportAssetsDatabase::getPrimaryInputFile(AssetType type, const String& assetId) const
{
	std::lock_guard<std::mutex> lock(mutex);

	if (const auto * entry = findEntry(type, assetId); entry) {
		const auto& asset = entry->asset;
		for (auto& o: asset.outputFiles) {
			if (o.type == type && o.name == assetId) {
				return o.primaryInputFile.isEmpty() ? asset.inputFiles.at(0).getPath() : o.primaryInputFile;
			}
		}
	}

	return {};
}

int64_t ImportAssetsDatabase::getAssetTimestamp(AssetType type, const String& assetId) const
{
	std::lock_guard<std::mutex> lock(mutex);

	if (const auto * entry = findEntry(type, assetId); entry) {
		return entry->asset.getLatestTimestamp();
	}

	return 0;
}

bool ImportAssetsDatabase::needsImporting(const ImportAssetsDatabaseEntry& asset, bool includeFailed) const
{
	std::lock_guard<std::mutex> lock(mutex);
	
	// Check if it failed loading last time
	auto iter = assetsFailed.find(asset.assetId);
	const bool failed = iter != assetsFailed.end();
	if (failed && includeFailed) {
		return true;
	}
	
	// Check if this was imported before
	if (!failed) {
		iter = assetsImported.find(asset.assetId);
		if (iter == assetsImported.end()) {
			// Asset didn't even exist before
			return true;
		}
	}

	// At this point, iter points to the failed one if it failed, or the the old successful one if it didn't.
	const auto& oldAsset = iter->second.asset;

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
	for (const auto& i: asset.inputFiles) {
		auto result = std::find_if(oldAsset.inputFiles.begin(), oldAsset.inputFiles.end(), [&](const AssetPath& entry) { return entry.getDataPath() == i.getDataPath(); });
		if (result == oldAsset.inputFiles.end()) {
			// File wasn't there before
			return true;
		} else if (result->getTimestamp() != i.getTimestamp()) {
			// Timestamp changed
			return true;
		}
	}

	// Any of the additional input files changed?
	for (const auto& i: oldAsset.additionalInputFiles) {
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
		for (const auto& o: oldAsset.outputFiles) {
			for (const auto& version: o.platformVersions) {
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
	indexDirty = true;
	
	auto failIter = assetsFailed.find(asset.assetId);
	if (failIter != assetsFailed.end()) {
		assetsFailed.erase(failIter);
	}
}

void ImportAssetsDatabase::markDeleted(const ImportAssetsDatabaseEntry& asset)
{
	std::lock_guard<std::mutex> lock(mutex);
	assetsImported.erase(asset.assetId);
	indexDirty = true;
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

Vector<ImportAssetsDatabaseEntry> ImportAssetsDatabase::getAllMissing() const
{
	std::lock_guard<std::mutex> lock(mutex);
	Vector<ImportAssetsDatabaseEntry> result;
	for (auto& e : assetsImported) {
		if (!e.second.present) {
			result.push_back(e.second.asset);
		}
	}
	return result;
}

Vector<AssetResource> ImportAssetsDatabase::getOutFiles(String assetId) const
{
	std::lock_guard<std::mutex> lock(mutex);
	auto iter = assetsImported.find(assetId);
	if (iter != assetsImported.end()) {
		return iter->second.asset.outputFiles;
	} else {
		return {};
	}
}

Vector<String> ImportAssetsDatabase::getInputFiles() const
{
	std::lock_guard<std::mutex> lock(mutex);
	Vector<String> result;
	for (auto& i: inputFiles) {
		result.push_back(i.first);
	}
	return result;
}

Vector<std::pair<AssetType, String>> ImportAssetsDatabase::getAssetsFromFile(const Path& inputFile)
{
	std::lock_guard<std::mutex> lock(mutex);
	Vector<std::pair<AssetType, String>> result;

	for (auto& a: assetsImported) {
		const auto& asset = a.second.asset;
		for (auto& in: asset.inputFiles) {
			if (in.getPath() == inputFile) {
				for (auto& out: asset.outputFiles) {
					if (out.primaryInputFile.isEmpty() || out.primaryInputFile == inputFile) {
						result.emplace_back(out.type, out.name);
					}
				}
			}
		}
	}

	return result;
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
		Vector<String> platformsRead;
		s >> platformsRead;
		if (platformsRead == platforms) {
			s >> assetsImported;
			s >> inputFiles;
			indexDirty = true;
		}
	}
}

const ImportAssetsDatabase::AssetEntry* ImportAssetsDatabase::findEntry(AssetType type, const String& id) const
{
	if (indexDirty) {
		assetIndex.clear();
		for (const auto& a: assetsImported) {
			for (const auto& o: a.second.asset.outputFiles) {
				assetIndex[std::pair(o.type, o.name)] = &a.second;				
			}
		}
		indexDirty = false;
	}

	const auto result = assetIndex.find(std::pair(type, id));
	if (result != assetIndex.end()) {
		return result->second;
	}
	return nullptr;
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
