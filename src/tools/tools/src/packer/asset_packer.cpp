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


bool AssetPackListing::Entry::operator==(const Entry& other) const
{
	return type == other.type
		&& name == other.name
		&& modified == other.modified
		&& *entryData == *other.entryData;
}

bool AssetPackListing::Entry::operator!=(const Entry& other) const
{
	return !(*this == other);
}

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
	entries.push_back(Entry{ type, name, &entry, modified });
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

Vector<String> AssetPacker::pack(Project& project, const std::optional<std::set<String>>& assetsToPack, const Vector<String>& deletedAssets, ProgressCallback progress)
{
	struct PlatformData {
		String platformId;
		std::unique_ptr<AssetDatabase> db;
		HashMap<String, AssetPackListing> packs;
	};
	Vector<PlatformData> platData;

	// Setup platforms
	const auto& platforms = project.getPlatforms();
	const std::optional<size_t> pcIdx = std_ex::find_index(platforms, "pc");
	platData.reserve(platforms.size());
	for (auto& platform: platforms) {
		auto& d = platData.emplace_back();
		d.platformId = platform;
	}

	// Generate databases
	Vector<Future<void>> pending;
	progress(0, "Generating asset databases");
	for (auto& p: platData) {
		pending += Concurrent::execute([&] () {
			p.db = project.getImportAssetsDatabase().makeAssetDatabase(p.platformId);
		});
	}
	Concurrent::waitAll(pending);
	pending.clear();

	// Generate pack lists
	progress(0.2f, "Generating pack lists");
	const auto manifest = AssetPackManifest(FileSystem::readFile(project.getAssetPackManifestPath()));
	for (auto& p: platData) {
		pending += Concurrent::execute([&] () {
			p.packs = sortIntoPacks(manifest, *p.db, assetsToPack, deletedAssets);
		});
	}
	Concurrent::waitAll(pending);
	pending.clear();
	
	// Remove duplicate packs between console and PC
	progress(0.4f, "Checking pack status and sorting");
	if (false && pcIdx) { // TODO: this eliminates unnecessary duplicated packs, but needs handling by the rest of pipeline
		auto& pcPlatData = platData[*pcIdx];
		for (auto& curPlatData: platData) {
			if (curPlatData.platformId != "pc") {
				std_ex::erase_if_key(curPlatData.packs, [&] (const String& packId) {
					auto pcPackIter = pcPlatData.packs.find(packId);
					if (pcPackIter != pcPlatData.packs.end()) {
						//Logger::logInfo("Skipping pack " + packId + " for " + curPlatData.platformId + " - same as PC");
						return pcPackIter->second == curPlatData.packs.at(packId);
					}
					return false;
				});
			}
		}
	}

	// Decide which packs need packing
	for (auto& p: platData) {
		std_ex::erase_if_key(p.packs, [&] (const String& packId) {
			auto& packList = p.packs.at(packId);
			return !needsPacking(project, p.platformId, packId, packList);
		});
		for (auto& [id, packList]: p.packs) {
			packList.sort();
		}
	}

	// Pack
	progress(0.5f, "Packing");
	const auto src = project.getUnpackedAssetsPath();
	for (auto& p: platData) {
		pending += Concurrent::execute([&] () {
			const auto dst = project.getPackedAssetsPath(p.platformId);
			generatePacks(project, p.platformId, p.packs, src, dst, [=] (float progress, const String& str) {
				
			});
		});
	}
	Concurrent::waitAll(pending);

	progress(0.99f, "Packed");
	Vector<String> packed;
	for (auto& p: platData) {
		for (const auto& [packId, packData]: p.packs) {
			if (!packed.contains(packId)) {
				packed += packId;
			}
		}
	}

	progress(1.0f, "Done");
	return packed;
}

HashMap<String, AssetPackListing> AssetPacker::sortIntoPacks(const AssetPackManifest& manifest, const AssetDatabase& srcAssetDb, const std::optional<std::set<String>>& assetsToPack, const Vector<String>& deletedAssets)
{
	std::array<char, 2048> buffer;

	HashMap<String, AssetPackListing> packs;

	for (auto typeName: EnumNames<AssetType>()()) {
		const auto type = fromString<AssetType>(typeName);
		auto& db = srcAssetDb.getDatabase(type);
		for (auto& assetEntry: db.getAssets()) {
			const auto assetName = String::concatInBuffer(buffer, typeName, ":", assetEntry.first);

			// Find which pack this asset goes into
			const auto& packEntry = manifest.getPack(assetName);
	
			// Retrieve pack
			auto iter = packs.find(packEntry.getName());
			if (iter == packs.end()) {
				// Pack doesn't exist yet, create it first
				const auto [iter2, modified] = packs.insert_or_assign(packEntry.getName(), AssetPackListing(packEntry.getName(), packEntry.getEncryptionKey()));
				iter = iter2;

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

	// Activate any packs that contain deleted assets
	for (auto& assetName: deletedAssets) {
		const auto& packEntry = manifest.getPack(assetName);
		if (const auto iter = packs.find(packEntry.getName()); iter != packs.end()) {
			// Pack found, so mark it as needing repacking
			iter->second.setActive(true);
		}
	}

	return packs;
}

bool AssetPacker::needsPacking(Project& project, const String& platformId, const String& packId, const AssetPackListing& packList)
{
	if (packId.isEmpty()) {
		Logger::logWarning("The following assets will not be packed:");
		for (const auto& entry: packList.getEntries()) {
			Logger::logWarning("  [" + toString(entry.type) + "] " + entry.name);
		}
		Logger::logWarning("-----------------------\n");
		return false;
	} else {
		// Only pack if this pack listing is active or if it doesn't exist
		const auto dst = project.getPackedAssetsPath(platformId);
		const auto dstPack = dst / packId + ".dat";
		return packList.isActive() || !FileSystem::exists(dstPack);
	}
}

void AssetPacker::generatePacks(Project& project, const String& platformId, HashMap<String, AssetPackListing> packs, const Path& src, const Path& dst, ProgressCallback progress)
{
	const size_t n = packs.size();
	size_t i = 0;
	for (auto& packListing: packs) {
		const auto dstPack = dst / packListing.first + ".dat";
		generatePack(project, platformId, packListing.first, packListing.second, src, dstPack, [=] (float p, const String& s)
		{
			progress((p + i) * (1.0f / n), s);
		});
		i++;
	}
}

void AssetPacker::generatePack(Project& project, const String& platformId, const String& packId, const AssetPackListing& packListing, const Path& src, const Path& dst, ProgressCallback progress)
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
		if (entry.modified || fs.hasCached(src / entry.entryData->path) || !oldPack) {
			// Read from cache or filesystem
			fileData = fs.readFileCopy(src / entry.entryData->path);
		} else {
			// Read from pack
			auto oldData = oldPack->getData(entry.name, entry.type, false);
			if (oldData) {
				auto data = dynamic_cast<ResourceDataStatic*>(oldData.get())->getSpan();
				fileData = Bytes(reinterpret_cast<const Byte*>(data.data()), reinterpret_cast<const Byte*>(data.data()) + data.size());
			} else {
				// Read from cache after all...
				fileData = fs.readFileCopy(src / entry.entryData->path);
			}
		}

		const size_t size = fileData.size();
		if (size == 0) {
			Logger::logError("Unable to pack: \"" + (src / entry.entryData->path) + "\". File not found or empty.");
			continue;
		}
		
		// Read data into pack data
		const size_t pos = data.size();
		data.reserve(nextPowerOf2(pos + size));
		data.resize(pos + size);
		memcpy(data.data() + pos, fileData.data(), size);

		db.addAsset(entry.name, entry.type, AssetDatabase::Entry(toString(pos) + ":" + toString(size), entry.entryData->meta));

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
		Logger::logInfo("- [" + platformId + "] Packed " + toString(packListing.getEntries().size()) + " entries on \"" + packId + "\" (" + String::prettySize(data.size()) + ").");
	} else {
		throw Exception("Unable to write pack file " + dst.getNativeString(), HalleyExceptions::Tools);
	}
}
