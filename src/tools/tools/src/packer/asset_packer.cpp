#include "halley/tools/packer/asset_packer.h"
#include "halley/core/resources/asset_database.h"
#include "halley/tools/file/filesystem.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/support/logger.h"
#include "halley/tools/packer/asset_pack_manifest.h"
#include "halley/resources/resource.h"
#include "halley/core/resources/asset_pack.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"
using namespace Halley;


bool AssetPackListing::Entry::operator<(const Entry& other) const
{
	return name < other.name;
}

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

const Vector<AssetPackListing::Entry>& AssetPackListing::getEntries() const
{
	return entries;
}

const String& AssetPackListing::getEncryptionKey() const
{
	return encryptionKey;
}

void AssetPackListing::setActive(bool a)
{
	active = a;
}

bool AssetPackListing::isActive() const
{
	return active;
}

void AssetPackListing::sort()
{
	std::sort(entries.begin(), entries.end());
}

void AssetPacker::pack(Project& project, std::optional<std::set<String>> assetsToPack, const Vector<String>& deletedAssets, ProgressCallback progress)
{
	const auto& platforms = project.getPlatforms();
	const size_t n = platforms.size();
	for (size_t i = 0; i < n; ++i) {
		packPlatform(project, assetsToPack, deletedAssets, platforms[i], [=] (float p, const String& s) {
			progress((p + i) * (1.0f / n), s);
		});
	}
}

void AssetPacker::packPlatform(Project& project, std::optional<std::set<String>> assetsToPack, const Vector<String>& deletedAssets, const String& platform, ProgressCallback progress)
{
	const auto src = project.getUnpackedAssetsPath();
	const auto dst = project.getPackedAssetsPath(platform);

	Logger::logInfo("Packing for platform \"" + platform + "\" at \"" + dst.string() + "\".");
	const auto db = project.getImportAssetsDatabase().makeAssetDatabase(platform);
	const auto manifest = AssetPackManifest(FileSystem::readFile(project.getAssetPackManifestPath()));

	// Sort into packs
	const std::map<String, AssetPackListing> packs = sortIntoPacks(manifest, *db, assetsToPack, deletedAssets);

	// Generate packs
	generatePacks(packs, src, dst, std::move(progress));
}

std::map<String, AssetPackListing> AssetPacker::sortIntoPacks(const AssetPackManifest& manifest, const AssetDatabase& srcAssetDb, std::optional<std::set<String>> assetsToPack, const Vector<String>& deletedAssets)
{
	std::map<String, AssetPackListing> packs;
	for (auto typeName: EnumNames<AssetType>()()) {
		const auto type = fromString<AssetType>(typeName);
		auto& db = srcAssetDb.getDatabase(type);
		for (auto& assetEntry: db.getAssets()) {
			const String assetName = String(typeName) + ":" + assetEntry.first;

			// Find which pack this asset goes into
			auto packEntry = manifest.getPack("~:" + assetName);
			String packName;
			String encryptionKey;
			if (packEntry) {
				packName = packEntry->get().getName();
				encryptionKey = packEntry->get().getEncryptionKey();
			}

			// Retrieve pack
			auto iter = packs.find(packName);
			if (iter == packs.end()) {
				// Pack doesn't exist yet, create it first
				packs[packName] = AssetPackListing(packName, encryptionKey);
				iter = packs.find(packName);

				// Initialise it to active if there's no asset list to pack
				if (!assetsToPack) {
					iter->second.setActive(true);
				}
			}

			// Activate the pack if this asset was actually supposed to be packed
			if (assetsToPack) {
				if (assetsToPack->find(assetName) != assetsToPack->end()) {
					iter->second.setActive(true);
				}
			}

			// Add file to pack
			iter->second.addFile(type, assetEntry.first, assetEntry.second);
		}
	}

	// Sort all packs
	for (auto& p: packs) {
		p.second.sort();
	}

	// Activate any packs that contain deleted assets
	for (auto& assetName: deletedAssets) {
		String packName;
		auto packEntry = manifest.getPack("~:" + assetName);
		if (packEntry) {
			packName = packEntry->get().getName();
		}

		auto iter = packs.find(packName);
		if (iter != packs.end()) {
			// Pack found, so mark it as needing repacking
			iter->second.setActive(true);
		}
	}

	return packs;
}

void AssetPacker::generatePacks(std::map<String, AssetPackListing> packs, const Path& src, const Path& dst, ProgressCallback progress)
{
	struct Entry {
		String name;
		AssetPackListing* listing = nullptr;
		Path dstPack;
	};
	Vector<Entry> toPack;
	
	for (auto& packListing: packs) {
		if (packListing.first.isEmpty()) {
			Logger::logWarning("The following assets will not be packed:");
			for (const auto& entry: packListing.second.getEntries()) {
				Logger::logWarning("  [" + toString(entry.type) + "] " + entry.name);
			}
			Logger::logWarning("-----------------------\n");
		} else {
			// Only pack if this pack listing is active or if it doesn't exist
			auto dstPack = dst / packListing.first + ".dat";
			if (packListing.second.isActive() || !FileSystem::exists(dstPack)) {
				toPack.push_back(Entry{ packListing.first, &packListing.second, dstPack });
			}
		}
	}

	size_t n = toPack.size();
	for (size_t i = 0; i < n; ++i) {
		generatePack(toPack[i].name, *toPack[i].listing, src, toPack[i].dstPack, [=] (float p, const String& s)
		{
			progress((p + i) * (1.0f / n), s);
		});
	}
}

void AssetPacker::generatePack(const String& packId, const AssetPackListing& packListing, const Path& src, const Path& dst, ProgressCallback progress)
{
	AssetPack pack;
	AssetDatabase& db = pack.getAssetDatabase();
	Bytes& data = pack.getData();

	const size_t n = packListing.getEntries().size();
	size_t i = 0;

	for (auto& entry: packListing.getEntries()) {
		//Logger::logDev("  [" + toString(entry.type) + "] " + entry.name);

		// Read original file
		auto fileData = FileSystem::readFile(src / entry.path);
		const size_t pos = data.size();
		const size_t size = fileData.size();
		if (size == 0) {
			throw Exception("Unable to pack: \"" + (src / entry.path) + "\". File not found or empty.", HalleyExceptions::Tools);
		}
		
		// Read data into pack data
		data.reserve(nextPowerOf2(pos + size));
		data.resize(pos + size);
		memcpy(data.data() + pos, fileData.data(), size);

		db.addAsset(entry.name, entry.type, AssetDatabase::Entry(toString(pos) + ":" + toString(size), entry.metadata));

		progress(float(i) / float(n), packId);
		i++;
	}

	if (!packListing.getEncryptionKey().isEmpty()) {
		Logger::logInfo("- Encrypting \"" + packId + "\"...");
		pack.encrypt(packListing.getEncryptionKey());
	}

	// Write pack
	FileSystem::writeFile(dst, pack.writeOut());
	Logger::logInfo("- Packed " + toString(packListing.getEntries().size()) + " entries on \"" + packId + "\" (" + String::prettySize(data.size()) + ").");
}
