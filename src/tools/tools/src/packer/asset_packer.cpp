#include "halley/tools/packer/asset_packer.h"
#include "halley/core/resources/asset_database.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file/byte_serializer.h"
#include "halley/support/logger.h"
#include "halley/tools/packer/asset_pack_manifest.h"
#include "halley/resources/resource.h"
using namespace Halley;


AssetPack::AssetPack()
{
}

AssetPack::AssetPack(String name, String encryptionKey)
	: name(name)
	, encryptionKey(encryptionKey)
{
}

void AssetPack::addFile(AssetType type, const String& name)
{
	entries.push_back(Entry{ type, name });
}

const std::vector<AssetPack::Entry>& AssetPack::getEntries() const
{
	return entries;
}

void AssetPacker::pack(const AssetPackManifest& manifest, const Path& assetsDir, const Path& dst)
{
	// Retrieve assets database
	auto assetDbData = FileSystem::readFile(assetsDir / "assets.db");
	if (assetDbData.empty()) {
		throw Exception("Unable to find assets.db at \"" + assetsDir.string() + "\"");
	}
	AssetDatabase assetDb = Deserializer::fromBytes<AssetDatabase>(assetDbData);

	// Sort into packs
	std::map<String, AssetPack> packs;
	for (auto typeName: EnumNames<AssetType>()()) {
		auto type = fromString<AssetType>(typeName);
		auto& db = assetDb.getDatabase(type);
		for (auto& rawAssetName: db.getAssets()) {
			String assetName = String(typeName) + ":" + rawAssetName.first;
			auto packEntry = manifest.getPack(assetName);
			String key;
			String encryptionKey;
			if (packEntry) {
				key = packEntry.get().get().getName();
				encryptionKey = packEntry.get().get().getEncryptionKey();
			}

			auto iter = packs.find(key);
			if (iter == packs.end()) {
				packs[key] = AssetPack(key, encryptionKey);
				iter = packs.find(key);
			}

			iter->second.addFile(type, rawAssetName.first);
		}
	}

	// Print packs
	for (auto& pack: packs) {
		Logger::logInfo("Pack \"" + pack.first + "\" --------------");
		for (auto& entry: pack.second.getEntries()) {
			Logger::logInfo("[" + toString(entry.type) + "] " + entry.name);
		}
		Logger::logInfo("-----------------------\n");
	}
}
