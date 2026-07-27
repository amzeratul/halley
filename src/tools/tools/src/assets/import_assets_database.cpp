#include <utility>
#include "halley/tools/assets/import_assets_database.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/resource_data.h"
#include "halley/time/stopwatch.h"
#include "halley/tools/file/filesystem.h"
#include "halley/tools/file/filesystem_cache.h"
#include "halley/utils/algorithm.h"

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

void ImportAssetsDatabaseEntry::addInputFile(TimestampedPath path)
{
	if (!std_ex::contains_if(inputFiles, [&] (const AssetPath& entry) { return entry.getPath() == path.first; })) {
		inputFiles.push_back(std::move(path));
	}
}

void ImportAssetsDatabaseEntry::addInputFile(TimestampedPath path, Path dataPath)
{
	if (!std_ex::contains_if(inputFiles, [&] (const AssetPath& entry) { return entry.getPath() == path.first; })) {
		inputFiles.emplace_back(std::move(path), std::move(dataPath));
	}
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

ImportAssetsDatabase::ImportAssetsDatabase(Path directory, Path dbFile, Path assetsDbFile, Vector<String> platforms, int version)
	: platforms(std::move(platforms))
	, directory(std::move(directory))
	, dbFile(std::move(dbFile))
	, assetsDbFile(std::move(assetsDbFile))
	, version(version)
{
	load();
}

void ImportAssetsDatabase::load()
{
	UniqueLock lock(mutex);
	auto data = FileSystem::readFile(dbFile);
	if (data.size() > 0) {
		auto s = Deserializer(data);
		deserialize(s);
	}
	dbDirty = false;
}

void ImportAssetsDatabase::save() const
{
	UniqueLock lock(mutex);
	if (dbDirty) {
		FileSystem::writeFile(dbFile, Serializer::toBytes(*this));
		dbDirty = false;
	}

	if (assetsDbDirty) {
		const auto pcAssetDatabase = doMakeAssetDatabase("pc");
		FileSystem::writeFile(assetsDbFile, Serializer::toBytes(*pcAssetDatabase));
		assetsDbDirty = false;
	}
}

void ImportAssetsDatabase::clear()
{
	UniqueLock lock(mutex);
	inputFiles.clear();
	assetsImported.clear();
	assetsFailed.clear();
	assetIndex.clear();
	dbDirty = true;
	assetsDbDirty = true;
}

const Metadata* ImportAssetsDatabase::markInputPresentIfUpToDate(const String& path, const std::array<int64_t, 3>& timestamps)
{
	UniqueLock lock(mutex);

	// Is it an unknown file?
	const auto iter = inputFiles.find(path);
	if (iter == inputFiles.end()) {
		return nullptr;
	}

	// Any of the timestamps changed?
	if (iter->second.timestamp != timestamps) {
		return nullptr;
	}

	iter->second.missing = false;
	return &iter->second.metadata;
}

const Metadata& ImportAssetsDatabase::setInputFileMetadata(const String& path, const std::array<int64_t, 3>& timestamps, Metadata data, Path basePath)
{
	UniqueLock lock(mutex);

	auto& input = inputFiles[path];
	input.timestamp = timestamps;
	input.metadata = std::move(data);
	input.basePath = std::move(basePath);
	input.missing = false;
	dbDirty = true;
	return input.metadata;
}

void ImportAssetsDatabase::markInputMissing(const Path& path)
{
	UniqueLock lock(mutex);
	auto& input = inputFiles[path.getStringView()];
	input.missing = true;
}

void ImportAssetsDatabase::markAllInputFilesAsMissing()
{
	UniqueLock lock(mutex);
	
	for (auto& i: inputFiles) {
		i.second.missing = true;
	}
}

bool ImportAssetsDatabase::purgeMissingInputs()
{
	UniqueLock lock(mutex);

	const bool modified = std_ex::erase_if_value(inputFiles, [&](const InputFileEntry& e)
	{
		return e.missing;
	});

	dbDirty = dbDirty || modified;
	return modified;
}

std::optional<Metadata> ImportAssetsDatabase::getMetadata(const Path& path) const
{
	UniqueLock lock(mutex);

	const auto iter = inputFiles.find(path.getStringView());
	if (iter == inputFiles.end()) {
		return {};
	} else {
		return iter->second.metadata;
	}
}

std::optional<Metadata> ImportAssetsDatabase::getMetadata(AssetType type, const String& assetId) const
{
	UniqueLock lock(mutex);

	if (const auto * entry = findEntry(type, assetId); entry) {
		const auto& asset = entry->asset;
		for (auto& o: asset.outputFiles) {
			if (o.type == type && o.name == assetId) {
				const auto inputFile = o.primaryInputFile.isEmpty() ? asset.inputFiles.at(0).getPath() : o.primaryInputFile;

				const auto iter = inputFiles.find(inputFile.getStringView());
				if (iter == inputFiles.end()) {
					return {};
				}
				return iter->second.metadata;
			}
		}
	}

	return {};
}

Path ImportAssetsDatabase::getPrimaryInputFile(AssetType type, const String& assetId, bool absolutePath) const
{
	UniqueLock lock(mutex);

	if (const auto* entry = findEntry(type, assetId); entry) {
		const auto& asset = entry->asset;
		for (const auto& o: asset.outputFiles) {
			if (o.type == type && o.name == assetId) {
				auto path = o.primaryInputFile.isEmpty() ? asset.inputFiles.at(0).getPath() : o.primaryInputFile;
				if (absolutePath) {
					return asset.srcDir / path;
				} else {
					return path;
				}
			}
		}
	}

	return {};
}

int64_t ImportAssetsDatabase::getAssetTimestamp(AssetType type, const String& assetId) const
{
	UniqueLock lock(mutex);

	if (const auto * entry = findEntry(type, assetId); entry) {
		return entry->asset.getLatestTimestamp();
	}

	return 0;
}

ImportAssetsDatabase::ImportAction ImportAssetsDatabase::checkNeedsImporting(const ImportAssetsDatabaseEntry& asset, FileSystemCache& fsCache) const
{
	UniqueLock lock(mutex);

	// Check if it failed loading last time
	const auto iter = assetsFailed.find(std::pair{ asset.assetType, asset.assetId });
	const bool failed = iter != assetsFailed.end();

	// Check if this was imported before
	const AssetEntry* oldAssetPtr = nullptr;
	if (failed) {
		oldAssetPtr = &iter->second;
	} else {
		const auto iter2 = assetsImported.find(std::pair{ asset.assetType, asset.assetId });
		if (iter2 == assetsImported.end()) {
			// Asset didn't even exist before
			return ImportAction::Import;
		}
		oldAssetPtr = &iter2->second;
	}

	// At this point, oldAssetPtr points to the failed one if it failed, or to the old successful one if it didn't.
	const auto& oldAsset = oldAssetPtr->asset;

	// Input directory changed?
	if (asset.srcDir != oldAsset.srcDir) {
		return ImportAction::Import;
	}

	// Total count of input files changed?
	if (asset.inputFiles.size() != oldAsset.inputFiles.size()) {
		return ImportAction::Import;
	}

	// Any of the input files changed?
	// Note: We don't have to check old files on new input, because the size matches and all entries matched.
	constexpr size_t linearSearchLimit = 8;
	if (asset.inputFiles.size() <= linearSearchLimit) {
		for (const auto& i: asset.inputFiles) {
			auto result = std::find_if(oldAsset.inputFiles.begin(), oldAsset.inputFiles.end(), [&](const AssetPath& entry) { return entry.getDataPath() == i.getDataPath(); });
			if (result == oldAsset.inputFiles.end()) {
				// File wasn't there before
				return ImportAction::Import;
			} else if (result->getTimestamp() != i.getTimestamp()) {
				// Timestamp changed
				return ImportAction::Import;
			}
		}
	} else {
		// Too many input files for a quadratic scan (e.g. big atlases), compare via a map
		HashMap<String, int64_t> oldInputFiles;
		oldInputFiles.reserve(oldAsset.inputFiles.size());
		for (const auto& entry: oldAsset.inputFiles) {
			oldInputFiles[entry.getDataPath().getStringView()] = entry.getTimestamp();
		}
		for (const auto& i: asset.inputFiles) {
			const auto result = oldInputFiles.find(i.getDataPath().getStringView());
			if (result == oldInputFiles.end() || result->second != i.getTimestamp()) {
				return ImportAction::Import;
			}
		}
	}

	// Any of the additional input files changed?
	for (const auto& i: oldAsset.additionalInputFiles) {
		if (!fsCache.exists(i.first)) {
			// File removed
			return ImportAction::Import;
		} else if (fsCache.getLastWriteTime(i.first) != i.second) {
			// Timestamp changed
			return ImportAction::Import;
		}
	}

	// Have any of the output files gone missing?
	if (!failed) {
		Path path;
		for (const auto& o: oldAsset.outputFiles) {
			for (const auto& version: o.platformVersions) {
				path.makeConcat(directory, version.second.filepath);
				if (!fsCache.exists(path)) {
					return ImportAction::Import;
				}
			}
		}
	}

	return failed ? ImportAction::RetryFailed : ImportAction::None;
}

void ImportAssetsDatabase::markAsImported(const ImportAssetsDatabaseEntry& asset)
{
	AssetEntry entry;
	entry.asset = asset;
	entry.present = true;

	UniqueLock lock(mutex);
	assetsImported[std::pair{ asset.assetType, asset.assetId }] = entry;
	indexDirty = true;
	dbDirty = true;
	assetsDbDirty = true;

	auto failIter = assetsFailed.find(std::pair{ asset.assetType, asset.assetId });
	if (failIter != assetsFailed.end()) {
		assetsFailed.erase(failIter);
	}
}

void ImportAssetsDatabase::markDeleted(const ImportAssetsDatabaseEntry& asset)
{
	UniqueLock lock(mutex);
	const auto key = std::pair{ asset.assetType, asset.assetId };
	if (assetsImported.erase(key) > 0) {
		dbDirty = true;
		assetsDbDirty = true;
	}
	assetsFailed.erase(key);
	indexDirty = true;
}

void ImportAssetsDatabase::markFailed(const ImportAssetsDatabaseEntry& asset)
{
	AssetEntry entry;
	entry.asset = asset;
	entry.present = true;

	UniqueLock lock(mutex);
	assetsFailed[std::pair{ asset.assetType, asset.assetId }] = entry;
}

void ImportAssetsDatabase::markAssetsAsStillPresent(const HashMap<std::pair<ImportAssetType, String>, ImportAssetsDatabaseEntry>& assets)
{
	UniqueLock lock(mutex);
	for (auto& e: assetsImported) {
		e.second.present = assets.find(e.first) != assets.end();
	}
}

Vector<ImportAssetsDatabaseEntry> ImportAssetsDatabase::getAllMissing() const
{
	UniqueLock lock(mutex);
	Vector<ImportAssetsDatabaseEntry> result;
	for (auto& e: assetsImported) {
		if (!e.second.present) {
			result.push_back(e.second.asset);
		}
	}
	return result;
}

bool ImportAssetsDatabase::hasFailedFiles() const
{
	UniqueLock lock(mutex);
	return !assetsFailed.empty();
}

std::pair<Path, Vector<Path>> ImportAssetsDatabase::getInputFiles(ImportAssetType assetType, const String& assetId) const
{
	UniqueLock lock(mutex);
	const auto iter = assetsImported.find(std::pair{ assetType, assetId });
	if (iter != assetsImported.end()) {
		Vector<Path> result;
		auto srcDir = iter->second.asset.srcDir;
		for (const auto& inputFile: iter->second.asset.inputFiles) {
			result.push_back(inputFile.getDataPath());
		}
		return { srcDir, std::move(result) };
	} else {
		return {};
	}
}

Vector<AssetResource> ImportAssetsDatabase::getOutFiles(ImportAssetType assetType, const String& assetId) const
{
	UniqueLock lock(mutex);
	auto iter = assetsImported.find(std::pair{ assetType, assetId });
	if (iter != assetsImported.end()) {
		return iter->second.asset.outputFiles;
	} else {
		return {};
	}
}

Vector<String> ImportAssetsDatabase::getAllInputFiles() const
{
	UniqueLock lock(mutex);
	Vector<String> result;
	for (auto& i: inputFiles) {
		result.push_back(i.first);
	}
	std::sort(result.begin(), result.end());
	return result;
}

Vector<std::pair<AssetType, String>> ImportAssetsDatabase::getAssetsFromFile(const Path& inputFile)
{
	if (inputFile.getNumberOfParts() >= 3 && inputFile.getPart(0) == ".." && inputFile.getPart(1) == "halley") {
		return getAssetsFromFile(inputFile.dropFront(3)); // e.g. ../halley/assets_src
	}

	UniqueLock lock(mutex);
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

	std::sort(result.begin(), result.end());

	return result;
}

void ImportAssetsDatabase::updateAdditionalFileCache()
{
	UniqueLock lock(mutex);
	assetsWithAdditionalFile.clear();
	for (auto& a : assetsImported) {
		const auto& asset = a.second.asset;

		for (const auto& additional : asset.additionalInputFiles) {
			assetsWithAdditionalFile[additional.first.getString()].push_back(a.first);
		}
	}
}

Vector<std::pair<Path, Path>> ImportAssetsDatabase::getFilesForAssetsThatHasAdditionalFile(const Path& srcPath, const Path& inputFile)
{
	UniqueLock lock(mutex);

	if (assetsWithAdditionalFile.empty()) {
		return {};
	}

	const auto iter = assetsWithAdditionalFile.find((srcPath / inputFile).getString());
	if (iter != assetsWithAdditionalFile.end()) {
		Vector<std::pair<Path, Path>> result;
		for (const auto& assetKey: iter->second) {
			const auto& asset = assetsImported.at(assetKey).asset;

			for (const auto& input: asset.inputFiles) {
				result.push_back(std::pair<Path, Path>(asset.srcDir, input.getDataPath()));
			}
		}
		return result;
	}

	return {};
}

void ImportAssetsDatabase::serialize(Serializer& s) const
{
	s << version;
	s << platforms;
	s << assetsImported;
	s << inputFiles;
}

void ImportAssetsDatabase::deserialize(Deserializer& s)
{
	int loadVersion;
	s >> loadVersion;
	if (version == loadVersion) {
		Vector<String> platformsRead;
		s >> platformsRead;
		if (platformsRead == platforms) {
			s >> assetsImported;
			s >> inputFiles;
			indexDirty = true;
		}
	}
}

void ImportAssetsDatabase::setPlatforms(Vector<String> platforms)
{
	if (platforms != this->platforms) {
		this->platforms = std::move(platforms);
		load();
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
	UniqueLock lock(mutex);
	return doMakeAssetDatabase(platform);
}

std::unique_ptr<AssetDatabase> ImportAssetsDatabase::doMakeAssetDatabase(const String& platform) const
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
