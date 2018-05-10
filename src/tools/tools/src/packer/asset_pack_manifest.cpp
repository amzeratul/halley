#include "halley/tools/packer/asset_pack_manifest.h"
#include "halley/file_formats/config_file.h"
#include "halley/tools/packer/asset_packer.h"
using namespace Halley;

AssetPackManifestEntry::AssetPackManifestEntry()
{
}

AssetPackManifestEntry::AssetPackManifestEntry(const ConfigNode& node)
{
	name = node["name"].asString();
	encryptionKey = node["encryptionKey"].asString("");
	if (node.hasKey("matches")) {
		for (auto& m: node["matches"].asSequence()) {
			matches.push_back(m.asString());
		}
	}
}

const String& AssetPackManifestEntry::getName() const
{
	return name;
}

bool AssetPackManifestEntry::checkMatch(const String& asset) const
{
	for (auto& m: matches) {
		if (asset.contains(m)) {
			return true;
		}
	}
	return false;
}

bool AssetPackManifestEntry::isEncrypted() const
{
	return !encryptionKey.isEmpty();
}

const String& AssetPackManifestEntry::getEncryptionKey() const
{
	return encryptionKey;
}

AssetPackManifest::AssetPackManifest(const ConfigFile& file)
{
	auto& root = file.getRoot();

	if (root.hasKey("exclude")) {
		for (auto& e: root["exclude"].asSequence()) {
			exclude.push_back(e.asString());
		}
	}

	if (root.hasKey("packs")) {
		for (auto& p: root["packs"].asSequence()) {
			packs.emplace_back(p);
		}
	}
}

Maybe<std::reference_wrapper<const AssetPackManifestEntry>> AssetPackManifest::getPack(const String& asset) const
{
	for (auto& e: exclude) {
		if (asset.contains(e)) {
			return {};
		}
	}
	for (auto& pack: packs) {
		if (pack.checkMatch(asset)) {
			return std::reference_wrapper<const AssetPackManifestEntry>(pack);
		}
	}
	return {};
}
