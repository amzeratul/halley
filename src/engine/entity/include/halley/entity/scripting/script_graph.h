#pragma once
#include "script_node_enums.h"
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
		struct PinConnection {
			OptionalLite<uint32_t> dstNode = {};
			uint8_t dstPin = 0;
			OptionalLite<uint8_t> entityIdx;

			PinConnection() = default;
			PinConnection(const ConfigNode& node);
			PinConnection(uint32_t dstNode, uint8_t dstPin);
			explicit PinConnection(OptionalLite<uint8_t> entityIdx);

			ConfigNode toConfigNode() const;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
		};
		
		struct Pin {
			Vector<PinConnection> connections;

			Pin() = default;
			Pin(const ConfigNode& node);
			ConfigNode toConfigNode() const;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
		};
		
		ScriptGraphNode();
		ScriptGraphNode(String type, Vector2f position);
		ScriptGraphNode(const ConfigNode& node);

		ConfigNode toConfigNode() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		Vector2f getPosition() const { return position; }
		void setPosition(Vector2f p) { position = p; }

		const String& getType() const { return type; }

		Vector<Pin>& getPins() { return pins; }
		const Vector<Pin>& getPins() const { return pins; }
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

		void assignType(const ScriptNodeTypeCollection& nodeTypeCollection) const;
		const IScriptNodeType& getNodeType() const;

		uint32_t getId() const { return id; }
		void setId(uint32_t i) { id = i; }

		ScriptNodePinType getPinType(uint8_t idx) const;

	private:
		Vector2f position;
		String type;
		ConfigNode settings;
		Vector<Pin> pins;
		uint32_t id = 0;
		mutable const IScriptNodeType* nodeType = nullptr;
	};
	
	class ScriptGraph : public Resource {
	public:
		ScriptGraph();
		ScriptGraph(const ConfigNode& node);
		ScriptGraph(const ConfigNode& node, const EntitySerializationContext& context);

		ConfigNode toConfigNode() const;
		ConfigNode toConfigNode(const EntitySerializationContext& context) const;
		String toYAML() const;

		static std::shared_ptr<ScriptGraph> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::ScriptGraph; }

		void reload(Resource&& resource) override;
		void makeDefault();

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void makeBaseGraph();

		const Vector<ScriptGraphNode>& getNodes() const { return nodes; }
		Vector<ScriptGraphNode>& getNodes() { return nodes; }

		OptionalLite<uint32_t> getStartNode() const;
		uint64_t getHash() const;

		bool connectPins(uint32_t srcNode, uint8_t srcPinN, uint32_t dstNode, uint8_t dstPin);
		bool connectPin(uint32_t srcNode, uint8_t srcPinN, EntityId target);
		bool disconnectPin(uint32_t nodeIdx, uint8_t pinN);
		bool disconnectPinIfSingleConnection(uint32_t nodeIdx, uint8_t pinN);
		void validateNodePins(uint32_t nodeIdx);

		void assignTypes(const ScriptNodeTypeCollection& nodeTypeCollection) const;
		void finishGraph();

		EntityId getEntityId(OptionalLite<uint8_t> idx) const;
		uint8_t addEntityId(EntityId id);
		void removeEntityId(EntityId id);

	private:
		Vector<ScriptGraphNode> nodes;
		Vector<EntityId> entityIds;
		uint64_t hash = 0;

		mutable uint64_t lastAssignTypeHash = 1;
	};

	template <>
    class ConfigNodeSerializer<ScriptGraph> {
    public:
		ConfigNode serialize(ScriptGraph script, const EntitySerializationContext& context);
		ScriptGraph deserialize(const EntitySerializationContext& context, const ConfigNode& node);
    };
}
