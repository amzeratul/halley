#pragma once

#include "halley/text/halleystring.h"
#include "halley/resources/resource.h"
#include "halley/core/resources/asset_database.h"
#include "halley/data_structures/maybe.h"
#include <set>

namespace Halley {
	class Project;
	class AssetPackManifest;
	class Path;
		
	class AssetPackListing {
	public:
		struct Entry {
			AssetType type;
			String name;
			String path;
			Metadata metadata;

			bool operator<(const Entry& other) const;
		};
		
		AssetPackListing();
		AssetPackListing(String name, String encryptionKey);
		
		void addFile(AssetType type, const String& name, const AssetDatabase::Entry& entry);
		const std::vector<Entry>& getEntries() const;
		const String& getEncryptionKey() const;
		
		void setActive(bool active);
		bool isActive() const;
		void sort();

	private:
		String name;
		String encryptionKey;

		bool active = false;

		std::vector<Entry> entries;
	};

	class AssetPacker {
	public:
		static void pack(Project& project, Maybe<std::set<String>> assetsToPack, const std::vector<String>& deletedAssets);
		static void packPlatform(Project& project, Maybe<std::set<String>> assetsToPack, const std::vector<String>& deletedAssets, const String& platform);

	private:
		static std::map<String, AssetPackListing> sortIntoPacks(const AssetPackManifest& manifest, const AssetDatabase& srcAssetDb, Maybe<std::set<String>> assetsToPack, const std::vector<String>& deletedAssets);
		static void generatePacks(std::map<String, AssetPackListing> packs, const Path& src, const Path& dst);
		static void generatePack(const String& packId, const AssetPackListing& pack, const Path& src, const Path& dst);
	};
}
