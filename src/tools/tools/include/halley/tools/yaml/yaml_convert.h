#pragma once

#include <gsl/gsl>
#include "halley/file_formats/config_file.h"
#include <yaml-cpp/node/node.h>

namespace Halley {
	class YAMLConvert {
	public:
		static ConfigNode parseYAMLNode(const YAML::Node& node);
		static void parseConfig(ConfigFile& config, gsl::span<const gsl::byte> data);
	};
}