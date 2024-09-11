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
			String name;
			String path;
			Metadata metadata;
			bool modified;

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

	private:
		String name;
		Vector<uint8_t> encryptionKey;

		bool active = false;

		Vector<Entry> entries;
	};

	class AssetPacker {
	public:
		using ProgressCallback = std::function<void(float, const String&)>;
		
		static Vector<String> pack(Project& project, std::optional<std::set<String>> assetsToPack, const Vector<String>& deletedAssets, ProgressCallback progress);
		static void packPlatform(Project& project, std::optional<std::set<String>> assetsToPack, const Vector<String>& deletedAssets, const String& platform, ProgressCallback progress, Vector<String>& packed);

	private:
		static std::map<String, AssetPackListing> sortIntoPacks(const AssetPackManifest& manifest, const AssetDatabase& srcAssetDb, std::optional<std::set<String>> assetsToPack, const Vector<String>& deletedAssets);
		static void generatePacks(Project& project, std::map<String, AssetPackListing> packs, const Path& src, const Path& dst, ProgressCallback progress, Vector<String>& packed);
		static void generatePack(Project& project, const String& packId, const AssetPackListing& pack, const Path& src, const Path& dst, ProgressCallback progress);
	};
}
