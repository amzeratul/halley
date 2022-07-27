#pragma once
#include "script_node_enums.h"
#include "script_state.h"
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
			OptionalLite<ScriptNodeId> dstNode = {};
			ScriptPinId dstPin = 0;
			OptionalLite<uint8_t> entityIdx;

			PinConnection() = default;
			PinConnection(const ConfigNode& node);
			PinConnection(ScriptNodeId dstNode, ScriptPinId dstPin);
			explicit PinConnection(OptionalLite<uint8_t> entityIdx);

			ConfigNode toConfigNode() const;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);

			bool hasConnection() const;
		};
		
		struct Pin {
			Vector<PinConnection> connections;

			Pin() = default;
			Pin(const ConfigNode& node);
			ConfigNode toConfigNode() const;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);

			bool hasConnection() const;
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

		void onNodeRemoved(ScriptNodeId nodeId);
		void remapNodes(const HashMap<ScriptNodeId, ScriptNodeId>& remap);
		void offsetNodes(ScriptNodeId offset);

		void assignType(const ScriptNodeTypeCollection& nodeTypeCollection) const;
		const IScriptNodeType& getNodeType() const;

		ScriptNodeId getId() const { return id; }
		void setId(ScriptNodeId i) { id = i; }
		OptionalLite<ScriptNodeId> getParentNode() const { return parentNode; }
		void setParentNode(OptionalLite<ScriptNodeId> id) { parentNode = id; }

		ScriptNodePinType getPinType(ScriptPinId idx) const;

	private:
		mutable const IScriptNodeType* nodeType = nullptr;
		ConfigNode settings;
		Vector<Pin> pins;
		String type;
		Vector2f position;
		ScriptNodeId id = 0;
		OptionalLite<ScriptNodeId> parentNode;
	};

	struct ScriptGraphNodeRoots {
		struct Entry {
			Range<ScriptNodeId> range;
			ScriptNodeId root;

			Entry() = default;
			Entry(Range<ScriptNodeId> range, ScriptNodeId root);
			Entry(const ConfigNode& node);
			ConfigNode toConfigNode() const;
		};

		Vector<Entry> mapping;

		ScriptGraphNodeRoots() = default;
		ScriptGraphNodeRoots(const ConfigNode& node);
		ConfigNode toConfigNode() const;

		void addRoot(ScriptNodeId id, ScriptNodeId root);
		ScriptNodeId getRoot(ScriptNodeId id) const;
	};
	
	class ScriptGraph : public Resource {
	public:
		struct FunctionParameters {
			uint8_t nOutput = 1;
			uint8_t nDataInput = 0;
			uint8_t nTargetInput = 0;
			uint8_t nDataOutput = 0;
			uint8_t nTargetOutput = 0;
		};

		ScriptGraph();
		ScriptGraph(const ConfigNode& node);
		ScriptGraph(const ConfigNode& node, const EntitySerializationContext& context);

		void load(const ConfigNode& node, const EntitySerializationContext& context);
		void loadDependencies(const Resources& resources);

		ConfigNode toConfigNode() const;
		ConfigNode toConfigNode(const EntitySerializationContext& context) const;
		String toYAML() const;

		Vector<String> getMessageNames() const;
		int getMessageNumParams(const String& messageId) const;

		static std::shared_ptr<ScriptGraph> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::ScriptGraph; }

		void reload(Resource&& resource) override;
		void makeDefault();

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void makeBaseGraph();

		const Vector<ScriptGraphNode>& getNodes() const { return nodes; }
		Vector<ScriptGraphNode>& getNodes() { return nodes; }

		OptionalLite<ScriptNodeId> getStartNode() const;
		OptionalLite<ScriptNodeId> getCallee(ScriptNodeId node) const;
		OptionalLite<ScriptNodeId> getCaller(ScriptNodeId node) const;
		OptionalLite<ScriptNodeId> getReturnTo(ScriptNodeId node) const;
		OptionalLite<ScriptNodeId> getReturnFrom(ScriptNodeId node) const;
		uint64_t getHash() const;

		std::optional<ScriptNodeId> getMessageInboxId(const String& messageId, bool requiresSpawningScript = false) const;

		bool connectPins(ScriptNodeId srcNode, ScriptPinId srcPinN, ScriptNodeId dstNode, ScriptPinId dstPin);
		bool connectPin(ScriptNodeId srcNode, ScriptPinId srcPinN, EntityId target);
		bool disconnectPin(ScriptNodeId nodeIdx, ScriptPinId pinN);
		bool disconnectPinIfSingleConnection(ScriptNodeId nodeIdx, ScriptPinId pinN);
		void validateNodePins(ScriptNodeId nodeIdx);

		void assignTypes(const ScriptNodeTypeCollection& nodeTypeCollection) const;
		void finishGraph();

		EntityId getEntityId(OptionalLite<uint8_t> idx) const;
		OptionalLite<uint8_t> getEntityIdx(EntityId id) const;
		uint8_t addEntityId(EntityId id);
		void removeEntityId(EntityId id);

		ScriptNodeId getNodeRoot(ScriptNodeId nodeId) const;
		const ScriptGraphNodeRoots& getRoots() const;
		void setRoots(ScriptGraphNodeRoots roots);

		FunctionParameters getFunctionParameters() const;

	private:
		Vector<ScriptGraphNode> nodes;
		Vector<EntityId> entityIds;
		Vector<std::pair<ScriptNodeId, ScriptNodeId>> callerToCallee;
		Vector<std::pair<ScriptNodeId, ScriptNodeId>> returnToCaller;
		uint64_t hash = 0;

		mutable uint64_t lastAssignTypeHash = 1;

		ScriptGraphNodeRoots roots;

		std::pair<ScriptNodeId, ScriptNodeId> appendGraph(ScriptNodeId parent, const ScriptGraph& other);
		ScriptNodeId findNodeRoot(ScriptNodeId nodeId) const;
		void generateRoots();
	};

	template <>
    class ConfigNodeSerializer<ScriptGraph> {
    public:
		ConfigNode serialize(ScriptGraph script, const EntitySerializationContext& context);
		ScriptGraph deserialize(const EntitySerializationContext& context, const ConfigNode& node);
		void deserialize(const EntitySerializationContext& context, const ConfigNode& node, ScriptGraph& target);
    };
}
