#include "halley/tools/yaml/yaml_convert.h"
#include "halley/bytes/byte_serializer.h"
#include "halley-yamlcpp.h"
#include "halley/tools/file/filesystem.h"
using namespace Halley;

ConfigNode YAMLConvert::parseYAMLNode(const YAML::Node& node)
{
	ConfigNode result;

	if (node.IsMap()) {
		std::map<String, ConfigNode> map;
		for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
			String key = it->first.as<std::string>();
			map[key] = parseYAMLNode(it->second);
		}
		result = std::move(map);
	}
	else if (node.IsSequence()) {
		std::vector<ConfigNode> list;
		for (auto& n : node) {
			list.emplace_back(parseYAMLNode(n));
		}
		result = std::move(list);
	}
	else if (node.IsScalar()) {
		auto str = String(node.as<std::string>());
		if (str.isNumber()) {
			if (str.isInteger()) {
				result = str.toInteger();
			} else {
				result = str.toFloat();
			}
		}
		else {
			result = str;
		}
	}

	result.setOriginalPosition(node.Mark().line, node.Mark().column);
	return result;
}

void YAMLConvert::parseConfig(ConfigFile& config, gsl::span<const gsl::byte> data)
{
	String strData(reinterpret_cast<const char*>(data.data()), data.size());
	YAML::Node root = YAML::Load(strData.cppStr());

	config.getRoot() = parseYAMLNode(root);
}