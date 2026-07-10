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
	encryptionKey = Encode::decodeBase64(node["encryptionKey"].asString(""));
	matches = node["matches"].asVector<String>({});
	prefixes = node["prefixes"].asVector<String>({});

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

bool AssetPackManifestEntry::isEncrypted() const
{
	return !encryptionKey.empty();
}

const Vector<uint8_t>& AssetPackManifestEntry::getEncryptionKey() const
{
	return encryptionKey;
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

std::optional<std::reference_wrapper<const AssetPackManifestEntry>> AssetPackManifest::getPack(std::string_view asset) const
{
	for (auto& e: exclude) {
		if (asset.find(e) != std::string_view::npos) {
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
