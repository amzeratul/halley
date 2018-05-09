#pragma once
#include "halley/text/halleystring.h"
#include "halley/data_structures/maybe.h"

namespace Halley {
	class ConfigNode;
	class ConfigFile;

	class AssetPackManifestEntry {
	public:
		AssetPackManifestEntry();
		AssetPackManifestEntry(const ConfigNode& node);

		const String& getName() const;
		bool checkMatch(const String& asset) const;
		bool isEncrypted() const;
		const String& getEncryptionKey() const;

	private:
		String name;
		String encryptionKey;
		std::vector<String> matches;
		bool encrypted = false;
	};

	class AssetPackManifest {
	public:
		explicit AssetPackManifest(const ConfigFile& file);

		Maybe<std::reference_wrapper<const AssetPackManifestEntry>> getPack(const String& asset) const;

	private:
		std::vector<String> exclude;
		std::vector<AssetPackManifestEntry> packs;
	};
}
