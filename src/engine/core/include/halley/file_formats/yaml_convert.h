#pragma once

#include <gsl/gsl>

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
			EmitOptions(Vector<String> mapKeyOrder = {}, bool compactMaps = false)
				: mapKeyOrder(std::move(mapKeyOrder))
				, compactMaps(compactMaps)
			{}

			Vector<String> mapKeyOrder;
			bool compactMaps;
		};

		class ParseOptions {
		public:
			ParseOptions(bool parseMapsAsSequencesOfPairs = false)
				: parseMapsAsSequencesOfPairs(parseMapsAsSequencesOfPairs)
			{}

			bool parseMapsAsSequencesOfPairs;
		};
		
		static void parseConfig(ConfigFile& config, gsl::span<const gsl::byte> data, const ParseOptions& options = {});
		static ConfigNode parseYAMLNode(const YAML::Node& node, const ParseOptions& options = {});
		static ConfigFile parseConfig(gsl::span<const gsl::byte> data, const ParseOptions& options = {});
		static ConfigFile parseConfig(const Bytes& data, const ParseOptions& options = {});
		static ConfigNode parseConfig(const String& str, const ParseOptions& options = {});
		static ConfigFile parseConfig(const Path& path, const ParseOptions& options = {});
		
		static String generateYAML(const ConfigFile& config, const EmitOptions& options = {});
		static String generateYAML(const ConfigNode& node, const EmitOptions& options = {});

	private:
		static void emitNode(const ConfigNode& node, YAML::Emitter& emitter, const EmitOptions& options);
		static void emitSequence(const ConfigNode& node, YAML::Emitter& emitter, const EmitOptions& options);
		static void emitMap(const ConfigNode& node, YAML::Emitter& emitter, const EmitOptions& options);
		static bool isCompactSequence(const ConfigNode& node, int depth, const EmitOptions& options);
	};

}