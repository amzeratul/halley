#include "halley/tools/assets/import_assets_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/resource_data.h"
#include "halley/file/filesystem.h"

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

ImportAssetsDatabase::ImportAssetsDatabase(Project& project, Path dbFile)
	: project(project)
	, dbFile(dbFile)
{
	load();
}

void ImportAssetsDatabase::load()
{
	std::lock_guard<std::mutex> lock(mutex);
	try {
		auto data = FileSystem::readFile(dbFile);
		auto s = Deserializer(data);
		deserialize(s);
	} catch (...) {
		// No database found, just ignore it
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
		return true;
	} else {
		auto& oldAsset = iter->second.asset;
		if (asset.srcDir != oldAsset.srcDir) {
			// Directory changed
			return true;
		}

		if (asset.inputFiles.size() != oldAsset.inputFiles.size()) {
			// Number of files changed
			return true;
		}

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

		// At this point, we know it's identical. We don't have to check old files on new input, because the size matches and all entries matched.

		return false;
	}
}

void ImportAssetsDatabase::markAsImported(const ImportAssetsDatabaseEntry& asset)
{
	std::lock_guard<std::mutex> lock(mutex);
	AssetEntry entry;
	entry.asset = asset;
	entry.present = true;
	assetsImported[asset.assetId] = entry;
}

void ImportAssetsDatabase::markDeleted(const ImportAssetsDatabaseEntry& asset)
{
	std::lock_guard<std::mutex> lock(mutex);
	assetsImported.erase(asset.assetId);
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

void ImportAssetsDatabase::serialize(Serializer& s) const
{
	int version = 1;
	s << version;
	s << assetsImported;
}

void ImportAssetsDatabase::deserialize(Deserializer& s)
{
	int version;
	s >> version;
	if (version == 1) {
		s >> assetsImported;
	} else {
		save();
	}
}
