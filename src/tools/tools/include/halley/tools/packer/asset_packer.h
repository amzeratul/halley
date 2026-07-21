#pragma once

#include "halley/text/halleystring.h"
#include "halley/resources/resource.h"
#include "halley/resources/asset_database.h"
#include "halley/data_structures/maybe.h"
#include <set>

#include "halley/utils/encrypt.h"

namespace Halley {
	class Project;
	class AssetPackManifest;
	class Path;
		
	class AssetPackListing {
	public:
		struct Entry {
			AssetType type;
			std::string_view name;
			const AssetDatabase::Entry* entryData;
			bool modified;

			bool operator==(const Entry& other) const;
			bool operator!=(const Entry& other) const;
			bool operator<(const Entry& other) const;
		};
		
		AssetPackListing();
		AssetPackListing(String name, Vector<uint8_t> encryptionKey);
		
		void addFile(AssetType type, const String& name, const AssetDatabase::Entry& entry, bool modified);
		const Vector<Entry>& getEntries() const;
		std::optional<Encrypt::AESKey> getEncryptionKey() const;
		
		void setActive(bool active);
		bool isActive() const;
		void sort();

		bool operator==(const AssetPackListing& other) const = default;
		bool operator!=(const AssetPackListing& other) const = default;

	private:
		String name;
		Vector<uint8_t> encryptionKey;

		bool active = false;

		Vector<Entry> entries;
	};

	class AssetPacker {
	public:
		using ProgressCallback = std::function<void(float, const String&)>;
		
		static Vector<String> pack(Project& project, const std::optional<std::set<String>>& assetsToPack, const Vector<String>& deletedAssets, ProgressCallback progress);

	private:
		static HashMap<String, AssetPackListing> sortIntoPacks(const AssetPackManifest& manifest, const AssetDatabase& srcAssetDb, const std::optional<std::set<String>>& assetsToPack, const Vector<String>& deletedAssets);
		static void generatePacks(Project& project, const String& platformId, HashMap<String, AssetPackListing> packs, const Path& src, const Path& dst, ProgressCallback progress);
		static void generatePack(Project& project, const String& platformId, const String& packId, const AssetPackListing& pack, const Path& src, const Path& dst, ProgressCallback progress);
		static bool needsPacking(Project& project, const String& platformId, const String& packId, const AssetPackListing& packList);
	};
}
