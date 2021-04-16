#pragma once
#include "halley/bytes/config_node_serializer.h"
#include "halley/utils/hash.h"

namespace Halley {
	class ScriptGraphNode {
	public:
		ScriptGraphNode();
		ScriptGraphNode(String type, Vector2f position);
		ScriptGraphNode(const ConfigNode& node);

		ConfigNode toConfigNode() const;

		Vector2f getPosition() const { return position; }
		void setPosition(Vector2f p) { position = p; }

		const String& getType() const { return type; }

		std::vector<uint32_t>& getOutput() { return output; }
		const std::vector<uint32_t>& getOutput() const { return output; }

		const ConfigNode& getSettings() const { return settings; }
		ConfigNode& getSettings() { return settings; }

		void setOutput(uint8_t outputPinN, uint32_t targetNode, uint8_t inputPinN);
		void feedToHash(Hash::Hasher& hasher);

	private:
		Vector2f position;
		String type;
		ConfigNode settings;
		std::vector<uint32_t> output;
	};
	
	class ScriptGraph {
	public:
		ScriptGraph();
		ScriptGraph(const ConfigNode& node);

		ConfigNode toConfigNode() const;

		const std::vector<ScriptGraphNode>& getNodes() const { return nodes; }
		std::vector<ScriptGraphNode>& getNodes() { return nodes; }

		OptionalLite<uint32_t> getStartNode() const;
		uint64_t getHash() const;

	private:
		std::vector<ScriptGraphNode> nodes;
		uint64_t hash;

		void computeHash();
	};

	template<>
	class ConfigNodeSerializer<ScriptGraph> {
	public:
		ConfigNode serialize(const ScriptGraph& graph, const ConfigNodeSerializationContext& context);
		ScriptGraph deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node);
	};
}
