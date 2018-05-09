#pragma once

#include "halley/text/halleystring.h"
#include "halley/resources/resource.h"
#include "halley/core/resources/asset_database.h"

namespace Halley {
	class AssetPackManifest;
	class Path;
		
	class AssetPack {
	public:
		struct Entry {
			AssetType type;
			String name;
			String path;
			Metadata metadata;
		};
		
		AssetPack();
		AssetPack(String name, String encryptionKey);
		
		void addFile(AssetType type, const String& name, const AssetDatabase::Entry& entry);
		const std::vector<Entry>& getEntries() const;

	private:
		String name;
		String encryptionKey;

		std::vector<Entry> entries;
	};

	class AssetPacker {
	public:
		static void pack(const AssetPackManifest& manifest, const Path& assetsDir, const Path& dst);

	private:
		static std::map<String, AssetPack> sortIntoPacks(const AssetPackManifest& manifest, const AssetDatabase& srcAssetDb);
		static void generatePacks(std::map<String, AssetPack> packs, const Path& src, const Path& dst);
		static void generatePack(AssetDatabase& db, const String& packId, const AssetPack& pack, const Path& src, const Path& dst);
	};
}
