#pragma once
#include "../entity_id.h"
#include "halley/bytes/config_node_serializer.h"
#include "halley/utils/hash.h"

namespace Halley {
	class ScriptGraphNode {
	public:
		struct Output {
			uint32_t nodeId = 0;
			uint8_t inputPin = 0;

			Output() = default;
			Output(const ConfigNode& node);
			ConfigNode toConfigNode() const;
		};
		
		ScriptGraphNode();
		ScriptGraphNode(String type, Vector2f position);
		ScriptGraphNode(const ConfigNode& node, const ConfigNodeSerializationContext& context);

		ConfigNode toConfigNode(const ConfigNodeSerializationContext& context) const;

		Vector2f getPosition() const { return position; }
		void setPosition(Vector2f p) { position = p; }

		const String& getType() const { return type; }

		std::vector<Output>& getOutputs() { return outputs; }
		const std::vector<Output>& getOutputs() const { return outputs; }

		std::vector<EntityId>& getTargets() { return targets; }
		const std::vector<EntityId>& getTargets() const { return targets; }

		const ConfigNode& getSettings() const { return settings; }
		ConfigNode& getSettings() { return settings; }

		void setOutput(uint8_t outputPinN, uint32_t targetNode, uint8_t inputPinN);
		void feedToHash(Hash::Hasher& hasher);

	private:
		Vector2f position;
		String type;
		ConfigNode settings;
		std::vector<Output> outputs;
		std::vector<EntityId> targets;
	};
	
	class ScriptGraph {
	public:
		ScriptGraph();
		ScriptGraph(const ConfigNode& node, const ConfigNodeSerializationContext& context);

		ConfigNode toConfigNode(const ConfigNodeSerializationContext& context) const;

		void makeBaseGraph();

		const std::vector<ScriptGraphNode>& getNodes() const { return nodes; }
		std::vector<ScriptGraphNode>& getNodes() { return nodes; }

		OptionalLite<uint32_t> getStartNode() const;
		uint64_t getHash() const;

	private:
		std::vector<ScriptGraphNode> nodes;
		uint64_t hash = 0;

		void computeHash();
	};

	template<>
	class ConfigNodeSerializer<ScriptGraphNode> {
	public:
		ConfigNode serialize(const ScriptGraphNode& node, const ConfigNodeSerializationContext& context);
		ScriptGraphNode deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node);
	};

	template<>
	class ConfigNodeSerializer<ScriptGraph> {
	public:
		ConfigNode serialize(const ScriptGraph& graph, const ConfigNodeSerializationContext& context);
		ScriptGraph deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node);
	};
}
