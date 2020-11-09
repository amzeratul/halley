#pragma once

#include <gsl/gsl>

#include "halley/core/editor_extensions/editor_custom_tools_interface.h"
#include "halley/file_formats/config_file.h"

namespace YAML {
	class Node;
	class Emitter;
}

namespace Halley {
	class Path;

	class YAMLConvert {
	public:
		class EmitOptions {
		public:
			std::vector<String> mapKeyOrder;
		};
		
		static void parseConfig(ConfigFile& config, gsl::span<const gsl::byte> data);
		static ConfigNode parseYAMLNode(const YAML::Node& node);
		static ConfigFile parseConfig(gsl::span<const gsl::byte> data);
		static ConfigFile parseConfig(const Bytes& data);

		static String generateYAML(const ConfigFile& config, const EmitOptions& options);
		static String generateYAML(const ConfigNode& node, const EmitOptions& options);

	private:
		static void emitNode(const ConfigNode& node, YAML::Emitter& emitter, const EmitOptions& options);
		static void emitSequence(const ConfigNode& node, YAML::Emitter& emitter, const EmitOptions& options);
		static void emitMap(const ConfigNode& node, YAML::Emitter& emitter, const EmitOptions& options);
		static bool isCompactSequence(const ConfigNode& node, int depth);
	};

}