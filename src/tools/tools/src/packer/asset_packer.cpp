#include "halley/tools/packer/asset_packer.h"
#include "halley/core/resources/asset_database.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file/byte_serializer.h"
#include "halley/support/logger.h"
#include "halley/tools/packer/asset_pack_manifest.h"
#include "halley/resources/resource.h"
#include "halley/core/resources/asset_pack.h"
using namespace Halley;


AssetPackListing::AssetPackListing()
{
}

AssetPackListing::AssetPackListing(String name, String encryptionKey)
	: name(name)
	, encryptionKey(encryptionKey)
{
}

void AssetPackListing::addFile(AssetType type, const String& name, const AssetDatabase::Entry& entry)
{
	entries.push_back(Entry{ type, name, entry.path, entry.meta });
}

const std::vector<AssetPackListing::Entry>& AssetPackListing::getEntries() const
{
	return entries;
}

void AssetPacker::pack(const AssetPackManifest& manifest, const Path& src, const Path& dst)
{
	// Retrieve assets database
	auto assetDbData = FileSystem::readFile(src / "assets.db");
	if (assetDbData.empty()) {
		throw Exception("Unable to find assets.db at \"" + src.string() + "\"");
	}
	AssetDatabase srcAssetDb = Deserializer::fromBytes<AssetDatabase>(assetDbData);

	// Sort into packs
	std::map<String, AssetPackListing> packs = sortIntoPacks(manifest, srcAssetDb);

	// Generate packs
	generatePacks(packs, src, dst);
}

std::map<String, AssetPackListing> AssetPacker::sortIntoPacks(const AssetPackManifest& manifest, const AssetDatabase& srcAssetDb)
{
	std::map<String, AssetPackListing> packs;
	for (auto typeName: EnumNames<AssetType>()()) {
		const auto type = fromString<AssetType>(typeName);
		auto& db = srcAssetDb.getDatabase(type);
		for (auto& assetEntry: db.getAssets()) {
			const String assetName = String(typeName) + ":" + assetEntry.first;
			auto packEntry = manifest.getPack(assetName);
			String key;
			String encryptionKey;
			if (packEntry) {
				key = packEntry.get().get().getName();
				encryptionKey = packEntry.get().get().getEncryptionKey();
			}

			auto iter = packs.find(key);
			if (iter == packs.end()) {
				packs[key] = AssetPackListing(key, encryptionKey);
				iter = packs.find(key);
			}

			iter->second.addFile(type, assetEntry.first, assetEntry.second);
		}
	}
	return packs;
}

void AssetPacker::generatePacks(std::map<String, AssetPackListing> packs, const Path& src, const Path& dst)
{
	for (auto& packListing: packs) {
		if (packListing.first.isEmpty()) {
			Logger::logWarning("The following assets will not be packed:");
			for (auto& entry: packListing.second.getEntries()) {
				Logger::logWarning("  [" + toString(entry.type) + "] " + entry.name);
			}
			Logger::logWarning("-----------------------\n");
		} else {
			generatePack(packListing.first, packListing.second, src, dst);
		}
	}
}

void AssetPacker::generatePack(const String& packId, const AssetPackListing& packListing, const Path& src, const Path& dst)
{
	AssetPack pack;
	AssetDatabase& db = pack.getAssetDatabase();
	Bytes& data = pack.getData();

	Logger::logInfo("Pack \"" + packId + "\" --------------");
	for (auto& entry: packListing.getEntries()) {
		Logger::logInfo("  [" + toString(entry.type) + "] " + entry.name);

		// Read original file
		auto fileData = FileSystem::readFile(src / entry.path);
		size_t pos = data.size();
		size_t size = fileData.size();
		String packPath = packId + ".dat";
		
		// Read data into pack data
		data.reserve(nextPowerOf2(pos + size));
		data.resize(pos + size);
		memcpy(data.data() + pos, fileData.data(), size);

		db.addAsset(entry.name, entry.type, AssetDatabase::Entry(toString(pos) + ":" + toString(size), entry.metadata));
	}
	Logger::logInfo("-----------------------\n");

	// Write pack
	FileSystem::writeFile(dst / packId + ".dat", Serializer::toBytes(pack));
}
