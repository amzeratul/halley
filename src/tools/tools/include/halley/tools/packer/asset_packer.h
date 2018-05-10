#pragma once

#include "halley/text/halleystring.h"
#include "halley/resources/resource.h"
#include "halley/core/resources/asset_database.h"

namespace Halley {
	class AssetPackManifest;
	class Path;
		
	class AssetPackListing {
	public:
		struct Entry {
			AssetType type;
			String name;
			String path;
			Metadata metadata;
		};
		
		AssetPackListing();
		AssetPackListing(String name, String encryptionKey);
		
		void addFile(AssetType type, const String& name, const AssetDatabase::Entry& entry);
		const std::vector<Entry>& getEntries() const;
		const String& getEncryptionKey() const;

	private:
		String name;
		String encryptionKey;

		std::vector<Entry> entries;
	};

	class AssetPacker {
	public:
		static void pack(const AssetPackManifest& manifest, const Path& assetsDir, const Path& dst);

	private:
		static std::map<String, AssetPackListing> sortIntoPacks(const AssetPackManifest& manifest, const AssetDatabase& srcAssetDb);
		static void generatePacks(std::map<String, AssetPackListing> packs, const Path& src, const Path& dst);
		static void generatePack(const String& packId, const AssetPackListing& pack, const Path& src, const Path& dst);
	};
}
