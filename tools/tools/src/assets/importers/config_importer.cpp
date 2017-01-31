#include "config_importer.h"
#include "halley/file/byte_serializer.h"
#include "halley/file_formats/config_file.h"
#include <yaml-cpp/yaml.h>
#include "halley/tools/file/filesystem.h"

using namespace Halley;

void ConfigImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	ConfigFile config;
	parseConfig(config, gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));
	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::ConfigFile, Serializer::toBytes(config));
}

static ConfigNode parseYAMLNode(const YAML::Node& node)
{
	ConfigNode result;

	if (node.IsMap()) {
		std::map<String, ConfigNode> map;
		for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
			String key = it->first.as<std::string>();
			map[key] = parseYAMLNode(it->second);
		}
		result = std::move(map);
	} else if (node.IsSequence()) {
		std::vector<ConfigNode> list;
		for (auto& n: node) {
			list.emplace_back(parseYAMLNode(n));
		}
		result = std::move(list);
	} else if (node.IsScalar()) {
		result = String(node.as<std::string>());
	}

	return result;
}

void ConfigImporter::parseConfig(ConfigFile& config, gsl::span<const gsl::byte> data)
{
	String strData(reinterpret_cast<const char*>(data.data()), data.size());
	YAML::Node root = YAML::Load(strData.cppStr());

	config.getRoot() = parseYAMLNode(root);
}
