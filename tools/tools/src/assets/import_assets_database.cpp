#include "halley/tools/assets/import_assets_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/resource_data.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

void ImportAssetsDatabaseEntry::serialize(Serializer& s) const
{
	s << assetId;
	s << srcDir;
	s << inputFiles;
	s << outputFiles;
	int t = int(assetType);
	s << t;
}

void ImportAssetsDatabaseEntry::deserialize(Deserializer& s)
{
	s >> assetId;
	s >> srcDir;
	s >> inputFiles;
	s >> outputFiles;
	int t;
	s >> t;
	assetType = AssetType(t);
}

void ImportAssetsDatabase::AssetEntry::serialize(Serializer& s) const
{
	s << asset;
}

void ImportAssetsDatabase::AssetEntry::deserialize(Deserializer& s)
{
	s >> asset;
}

ImportAssetsDatabase::ImportAssetsDatabase(Path directory, Path dbFile)
	: directory(directory)
	, dbFile(dbFile)
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
}

bool ImportAssetsDatabase::needsImporting(const ImportAssetsDatabaseEntry& asset) const
{
	std::lock_guard<std::mutex> lock(mutex);
	auto iter = assetsImported.find(asset.assetId);
	if (iter == assetsImported.end()) {
		// Asset didn't even exist before
		return true;
	} else {
		// Asset exists, check if it failed loading last time
		auto failIter = assetsFailed.find(asset.assetId);
		bool failed = failIter != assetsFailed.end();

		auto& oldAsset = (failed ? failIter : iter)->second.asset;

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
			auto result = std::find_if(oldAsset.inputFiles.begin(), oldAsset.inputFiles.end(), [&](const ImportAssetsDatabaseEntry::InputFile& entry) { return entry.first == i.first; });
			if (result == oldAsset.inputFiles.end()) {
				// File wasn't there before
				return true;
			} else if (result->second != i.second) {
				// Timestamp changed
				return true;
			}
		}

		// Have any of the output files gone missing?
		if (!failed) {
			for (auto& o: oldAsset.outputFiles) {
				if (!FileSystem::exists(directory / o)) {
					return true;
				}
			}
		}

		return false;
	}
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

void ImportAssetsDatabase::markAllInputsAsMissing()
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& e: assetsImported) {
		e.second.present = false;
	}
}

void ImportAssetsDatabase::markInputAsPresent(const ImportAssetsDatabaseEntry& asset)
{
	std::lock_guard<std::mutex> lock(mutex);
	auto iter = assetsImported.find(asset.assetId);
	if (iter != assetsImported.end()) {
		iter->second.present = true;
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

std::vector<Path> ImportAssetsDatabase::getOutFiles(String assetId) const
{
	auto iter = assetsImported.find(assetId);
	if (iter != assetsImported.end()) {
		return iter->second.asset.outputFiles;
	} else {
		return {};
	}
}

constexpr static int currentAssetVersion = 3;

void ImportAssetsDatabase::serialize(Serializer& s) const
{
	int version = currentAssetVersion;
	s << version;
	s << assetsImported;
}

void ImportAssetsDatabase::deserialize(Deserializer& s)
{
	int version;
	s >> version;
	if (version == currentAssetVersion) {
		s >> assetsImported;
	}
}
