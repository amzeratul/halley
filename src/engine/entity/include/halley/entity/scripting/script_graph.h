#pragma once
#include "../entity_id.h"
#include "halley/bytes/config_node_serializer.h"
#include "halley/utils/hash.h"

namespace Halley {
	class IScriptNodeType;
	class ScriptNodeTypeCollection;
	class ScriptGraph;
	class World;
	
	class ScriptGraphNode {
	public:
		struct Pin {
			OptionalLite<uint32_t> dstNode = {};
			uint8_t dstPin = 0;
			EntityId entity;

			Pin() = default;
			Pin(const ConfigNode& node, const ConfigNodeSerializationContext& context);
			ConfigNode toConfigNode(const ConfigNodeSerializationContext& context) const;
		};
		
		ScriptGraphNode();
		ScriptGraphNode(String type, Vector2f position);
		ScriptGraphNode(const ConfigNode& node, const ConfigNodeSerializationContext& context);

		ConfigNode toConfigNode(const ConfigNodeSerializationContext& context) const;

		Vector2f getPosition() const { return position; }
		void setPosition(Vector2f p) { position = p; }

		const String& getType() const { return type; }

		std::vector<Pin>& getPins() { return pins; }
		const std::vector<Pin>& getPins() const { return pins; }
		Pin& getPin(size_t idx)
		{
			if (idx >= pins.size()) {
				pins.resize(idx + 1);
			}
			return pins[idx];
		}
		const Pin& getPin(size_t idx) const
		{
			if (idx >= pins.size()) {
				static Pin dummy;
				return dummy;
			}
			return pins[idx];
		}

		const ConfigNode& getSettings() const { return settings; }
		ConfigNode& getSettings() { return settings; }

		void feedToHash(Hash::Hasher& hasher);

		void onNodeRemoved(uint32_t nodeId);
		bool disconnectPinsTo(uint32_t nodeId, OptionalLite<uint8_t> pinId);

		String getTargetName(const World& world, uint8_t idx) const;

		void assignType(const ScriptNodeTypeCollection& nodeTypeCollection) const;
		const IScriptNodeType& getNodeType() const;

		uint32_t getId() const { return id; }
		void setId(uint32_t i) { id = i; }

	private:
		Vector2f position;
		String type;
		ConfigNode settings;
		std::vector<Pin> pins;
		uint32_t id = 0;
		mutable const IScriptNodeType* nodeType = nullptr;
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

		bool connectPins(uint32_t srcNode, uint8_t srcPinN, uint32_t dstNode, uint8_t dstPin);
		bool connectPin(uint32_t srcNode, uint8_t srcPinN, EntityId target);
		bool disconnectPin(uint32_t node, uint8_t pin);

		void assignTypes(const ScriptNodeTypeCollection& nodeTypeCollection) const;

	private:
		std::vector<ScriptGraphNode> nodes;
		uint64_t hash = 0;

		mutable uint64_t lastAssignTypeHash = 1;

		void finishGraph();
	};

	template<>
	class ConfigNodeSerializer<ScriptGraphNode::Pin> {
	public:
		ConfigNode serialize(const ScriptGraphNode::Pin& pin, const ConfigNodeSerializationContext& context);
		ScriptGraphNode::Pin deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node);
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
