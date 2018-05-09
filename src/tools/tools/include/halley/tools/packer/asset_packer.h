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
		};
		
		AssetPack();
		AssetPack(String name, String encryptionKey);
		
		void addFile(AssetType type, const String& name);
		const std::vector<Entry>& getEntries() const;

	private:
		String name;
		String encryptionKey;

		std::vector<Entry> entries;
	};

	class AssetPacker {
	public:
		static void pack(const AssetPackManifest& manifest, const Path& assetsDir, const Path& dst);
	};
}
