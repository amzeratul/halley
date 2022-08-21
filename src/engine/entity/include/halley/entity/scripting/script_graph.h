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
			OptionalLite<GraphNodeId> dstNode = {};
			GraphPinId dstPin = 0;
			OptionalLite<uint8_t> entityIdx;

			PinConnection() = default;
			PinConnection(const ConfigNode& node);
			PinConnection(GraphNodeId dstNode, GraphPinId dstPin);
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

		void onNodeRemoved(GraphNodeId nodeId);
		void remapNodes(const HashMap<GraphNodeId, GraphNodeId>& remap);
		void offsetNodes(GraphNodeId offset);

		void assignType(const ScriptNodeTypeCollection& nodeTypeCollection) const;
		const IScriptNodeType& getNodeType() const;

		GraphNodeId getId() const { return id; }
		void setId(GraphNodeId i) { id = i; }
		OptionalLite<GraphNodeId> getParentNode() const { return parentNode; }
		void setParentNode(OptionalLite<GraphNodeId> id) { parentNode = id; }

		GraphNodePinType getPinType(GraphPinId idx) const;

	private:
		mutable const IScriptNodeType* nodeType = nullptr;
		ConfigNode settings;
		Vector<Pin> pins;
		String type;
		Vector2f position;
		GraphNodeId id = 0;
		OptionalLite<GraphNodeId> parentNode;
	};

	struct ScriptGraphNodeRoots {
		struct Entry {
			Range<GraphNodeId> range;
			GraphNodeId root;

			Entry() = default;
			Entry(Range<GraphNodeId> range, GraphNodeId root);
			Entry(const ConfigNode& node);
			ConfigNode toConfigNode() const;
		};

		Vector<Entry> mapping;

		ScriptGraphNodeRoots() = default;
		ScriptGraphNodeRoots(const ConfigNode& node);
		ConfigNode toConfigNode() const;

		void addRoot(GraphNodeId id, GraphNodeId root);
		GraphNodeId getRoot(GraphNodeId id) const;
		void clear();
	};
	
	class ScriptGraph : public Resource, public std::enable_shared_from_this<ScriptGraph> {
	public:
		struct FunctionParameters {
			uint8_t nOutput = 1;
			uint8_t nDataInput = 0;
			uint8_t nTargetInput = 0;
			uint8_t nDataOutput = 0;
			uint8_t nTargetOutput = 0;
			Vector<String> inputNames;
			Vector<String> outputNames;
			String icon;
		};

		ScriptGraph();
		ScriptGraph(const ConfigNode& node);
		ScriptGraph(const ConfigNode& node, const EntitySerializationContext& context);

		void load(const ConfigNode& node, const EntitySerializationContext& context);

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

		OptionalLite<GraphNodeId> getStartNode() const;
		OptionalLite<GraphNodeId> getCallee(GraphNodeId node) const;
		OptionalLite<GraphNodeId> getCaller(GraphNodeId node) const;
		OptionalLite<GraphNodeId> getReturnTo(GraphNodeId node) const;
		OptionalLite<GraphNodeId> getReturnFrom(GraphNodeId node) const;
		uint64_t getHash() const;

		std::optional<GraphNodeId> getMessageInboxId(const String& messageId, bool requiresSpawningScript = false) const;

		bool connectPins(GraphNodeId srcNode, GraphPinId srcPinN, GraphNodeId dstNode, GraphPinId dstPin);
		bool connectPin(GraphNodeId srcNode, GraphPinId srcPinN, EntityId target);
		bool disconnectPin(GraphNodeId nodeIdx, GraphPinId pinN);
		bool disconnectPinIfSingleConnection(GraphNodeId nodeIdx, GraphPinId pinN);
		void validateNodePins(GraphNodeId nodeIdx);

		void assignTypes(const ScriptNodeTypeCollection& nodeTypeCollection) const;
		void finishGraph();

		EntityId getEntityId(OptionalLite<uint8_t> idx) const;
		OptionalLite<uint8_t> getEntityIdx(EntityId id) const;
		uint8_t addEntityId(EntityId id);
		void removeEntityId(EntityId id);

		GraphNodeId getNodeRoot(GraphNodeId nodeId) const;
		const ScriptGraphNodeRoots& getRoots() const;
		void setRoots(ScriptGraphNodeRoots roots);

		void appendGraph(GraphNodeId parent, const ScriptGraph& other);
		Vector<int> getSubGraphIndicesForAssetId(const String& id) const;
		Range<GraphNodeId> getSubGraphRange(int subGraphIdx) const;

		FunctionParameters getFunctionParameters() const;

	private:
		Vector<ScriptGraphNode> nodes;
		Vector<EntityId> entityIds;
		Vector<std::pair<GraphNodeId, GraphNodeId>> callerToCallee;
		Vector<std::pair<GraphNodeId, GraphNodeId>> returnToCaller;
		Vector<std::pair<String, Range<GraphNodeId>>> subGraphs;
		uint64_t hash = 0;

		mutable uint64_t lastAssignTypeHash = 1;

		ScriptGraphNodeRoots roots;

		GraphNodeId findNodeRoot(GraphNodeId nodeId) const;
		void generateRoots();
		[[nodiscard]] bool isMultiConnection(GraphNodePinType pinType) const;
	};

	template <>
    class ConfigNodeSerializer<ScriptGraph> {
    public:
		ConfigNode serialize(ScriptGraph script, const EntitySerializationContext& context);
		ScriptGraph deserialize(const EntitySerializationContext& context, const ConfigNode& node);
		void deserialize(const EntitySerializationContext& context, const ConfigNode& node, ScriptGraph& target);
    };
}
