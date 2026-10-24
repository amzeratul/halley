#include "halley/tools/packer/asset_pack_manifest.h"
#include "halley/file_formats/config_file.h"
#include "halley/tools/packer/asset_packer.h"
#include <yaml-cpp/yaml.h>
#include "halley/file_formats/yaml_convert.h"
#include "halley/text/encode.h"
using namespace Halley;

AssetPackManifestEntry::AssetPackManifestEntry()
{
}

AssetPackManifestEntry::AssetPackManifestEntry(const ConfigNode& node)
{
	name = node["name"].asString();
	matches = node["matches"].asVector<String>({});
	prefixes = node["prefixes"].asVector<String>({});
	pack = node["pack"].asBool(true);

	if (node.hasKey("encryptionKeys")) {
		for (const auto& [k, v] : node["encryptionKeys"].asMap()) {
			encryptionKeys[k] = Encode::decodeBase64(v.asString(""));
		}
	}

	if (node.hasKey("encryptionKey")) {
		Logger::logError("Asset manifest no longer supports global encryptionKey; please specify one per platform instead");
	}

	for (auto& m: matches) {
		if (m.startsWith("~:")) {
			Logger::logError("Asset manifest matching on ~: is deprecated, use prefixes instead");
			m = m.substr(2);
		}
	}
}

const String& AssetPackManifestEntry::getName() const
{
	return name;
}

bool AssetPackManifestEntry::checkMatch(std::string_view asset) const
{
	for (auto& p: prefixes) {
		if (asset.starts_with(p)) {
			return true;
		}
	}
	for (auto& m: matches) {
		if (asset.find(m) != std::string_view::npos) {
			return true;
		}
	}
	return false;
}

bool AssetPackManifestEntry::isEncrypted(std::string_view platformId) const
{
	if (auto iter = encryptionKeys.find(platformId); iter != encryptionKeys.end()) {
		return !iter->second.empty();
	}
	return false;
}

const HashMap<String, Vector<uint8_t>>& AssetPackManifestEntry::getEncryptionKeys() const
{
	return encryptionKeys;
}

bool AssetPackManifestEntry::shouldPack() const
{
	return pack;
}

AssetPackManifest::AssetPackManifest(const Bytes& data)
{
	load(YAMLConvert::parseConfig(data));
}

AssetPackManifest::AssetPackManifest(const ConfigFile& file)
{
	load(file);
}

void AssetPackManifest::load(const ConfigFile& file)
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

const AssetPackManifestEntry& AssetPackManifest::getPack(std::string_view asset) const
{
	for (auto& e: exclude) {
		if (asset.find(e) != std::string_view::npos) {
			return emptyPack;
		}
	}
	for (auto& pack: packs) {
		if (pack.checkMatch(asset)) {
			return pack;
		}
	}
	return emptyPack;
}
