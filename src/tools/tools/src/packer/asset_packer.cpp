#include "halley/tools/packer/asset_packer.h"
#include "halley/resources/asset_database.h"
#include "halley/tools/file/filesystem.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/support/logger.h"
#include "halley/tools/packer/asset_pack_manifest.h"
#include "halley/resources/resource.h"
#include "halley/resources/asset_pack.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/file/filesystem_cache.h"
#include "halley/utils/algorithm.h"
using namespace Halley;


bool AssetPackListing::Entry::operator<(const Entry& other) const
{
	return std::tie(name, type) < std::tie(other.name, other.type);
}

AssetPackListing::AssetPackListing()
{
}

AssetPackListing::AssetPackListing(String name, Vector<uint8_t> encryptionKey)
	: name(std::move(name))
	, encryptionKey(std::move(encryptionKey))
{
}

void AssetPackListing::addFile(AssetType type, const String& name, const AssetDatabase::Entry& entry, bool modified)
{
	entries.push_back(Entry{ type, name, entry.path, entry.meta, modified });
}

const Vector<AssetPackListing::Entry>& AssetPackListing::getEntries() const
{
	return entries;
}

std::optional<Encrypt::AESKey> AssetPackListing::getEncryptionKey() const
{
	if (encryptionKey.empty()) {
		return std::nullopt;
	}
	return encryptionKey.const_span_size<16>();
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

Vector<String> AssetPacker::pack(Project& project, std::optional<std::set<String>> assetsToPack, const Vector<String>& deletedAssets, ProgressCallback progress)
{
	Vector<String> packed;
	const auto& platforms = project.getPlatforms();
	const size_t n = platforms.size();
	for (size_t i = 0; i < n; ++i) {
		packPlatform(project, assetsToPack, deletedAssets, platforms[i], [=] (float p, const String& s) {
			progress((p + i) * (1.0f / n), s);
		}, packed);
	}
	return packed;
}

void AssetPacker::packPlatform(Project& project, std::optional<std::set<String>> assetsToPack, const Vector<String>& deletedAssets, const String& platform, ProgressCallback progress, Vector<String>& packed)
{
	const auto src = project.getUnpackedAssetsPath();
	const auto dst = project.getPackedAssetsPath(platform);

	Logger::logInfo("Packing for platform \"" + platform + "\" at \"" + dst.string() + "\".");
	const auto db = project.getImportAssetsDatabase().makeAssetDatabase(platform);
	const auto manifest = AssetPackManifest(FileSystem::readFile(project.getAssetPackManifestPath()));

	// Sort into packs
	const std::map<String, AssetPackListing> packs = sortIntoPacks(manifest, *db, assetsToPack, deletedAssets);

	// Generate packs
	generatePacks(project, packs, src, dst, std::move(progress), packed);
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
			Vector<uint8_t> encryptionKey;
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
			bool fileModified = false;
			if (assetsToPack) {
				if (assetsToPack->find(assetName) != assetsToPack->end()) {
					iter->second.setActive(true);
					fileModified = true;
				}
			} else {
				fileModified = true;
			}

			// Add file to pack
			iter->second.addFile(type, assetEntry.first, assetEntry.second, fileModified);
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

void AssetPacker::generatePacks(Project& project, std::map<String, AssetPackListing> packs, const Path& src, const Path& dst, ProgressCallback progress, Vector<String>& packed)
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
		if (!std_ex::contains(packed, toPack[i].name)) {
			packed.push_back(toPack[i].name);
		}
		generatePack(project, toPack[i].name, *toPack[i].listing, src, toPack[i].dstPack, [=] (float p, const String& s)
		{
			progress((p + i) * (1.0f / n), s);
		});
	}
}

void AssetPacker::generatePack(Project& project, const String& packId, const AssetPackListing& packListing, const Path& src, const Path& dst, ProgressCallback progress)
{
	AssetPack pack;
	AssetDatabase& db = pack.getAssetDatabase();
	Bytes& data = pack.getData();
	auto& fs = project.getFileSystemCache();

	// Read old version of this pack, if available
	std::unique_ptr<AssetPack> oldPack;
	auto reader = std::make_unique<ResourceDataReaderFileSystem>(dst);
	if (reader->size() > 0) {
		try {
			oldPack = std::make_unique<AssetPack>(std::move(reader), packListing.getEncryptionKey(), true);
		} catch (...) {
			// Just ignore it if it fails to load asset pack for whatever reason
		}
	}
	reader = {};

	const size_t n = packListing.getEntries().size();
	size_t i = 0;

	for (auto& entry: packListing.getEntries()) {
		// Read original file
		// Priority:
		// 1. Cache
		// 2. Old pack
		// 3. Filesystem (via cache)
		Bytes fileData;
		if (entry.modified || fs.hasCached(src / entry.path) || !oldPack) {
			// Read from cache or filesystem
			fileData = fs.readFile(src / entry.path);
		} else {
			// Read from pack
			auto oldData = oldPack->getData(entry.name, entry.type, false);
			if (oldData) {
				auto data = dynamic_cast<ResourceDataStatic*>(oldData.get())->getSpan();
				fileData = Bytes(reinterpret_cast<const Byte*>(data.data()), reinterpret_cast<const Byte*>(data.data()) + data.size());
			} else {
				// Read from cache after all...
				fileData = fs.readFile(src / entry.path);
			}
		}

		const size_t size = fileData.size();
		if (size == 0) {
			Logger::logError("Unable to pack: \"" + (src / entry.path) + "\". File not found or empty.");
			continue;
		}
		
		// Read data into pack data
		const size_t pos = data.size();
		data.reserve(nextPowerOf2(pos + size));
		data.resize(pos + size);
		memcpy(data.data() + pos, fileData.data(), size);

		db.addAsset(entry.name, entry.type, AssetDatabase::Entry(toString(pos) + ":" + toString(size), entry.metadata));

		progress(float(i) / float(n), packId);
		i++;
	}

	oldPack = {}; // Release file handle!

	if (packListing.getEncryptionKey().has_value()) {
		Logger::logInfo("- Encrypting \"" + packId + "\"...");
		pack.encrypt(*packListing.getEncryptionKey());
	}

	// Write pack
	const auto packData = pack.writeOut();
	bool packed = FileSystem::writeFile(dst, packData);
	if (!packed) {
		// Try again
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(200ms);
		packed = FileSystem::writeFile(dst, packData);
	}

	if (packed) {
		Logger::logInfo("- Packed " + toString(packListing.getEntries().size()) + " entries on \"" + packId + "\" (" + String::prettySize(data.size()) + ").");
	} else {
		throw Exception("Unable to write pack file " + dst.getNativeString(), HalleyExceptions::Tools);
	}
}
