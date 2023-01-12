#pragma once
#include "script_node_enums.h"
#include "script_state.h"
#include "../entity_id.h"
#include "halley/bytes/config_node_serializer.h"
#include "halley/core/graph/base_graph.h"
#include "halley/utils/hash.h"

namespace Halley {
	class IScriptNodeType;
	class ScriptNodeTypeCollection;
	class ScriptGraph;
	class World;

	class ScriptGraphNode final : public BaseGraphNode {
	public:
		ScriptGraphNode();
		ScriptGraphNode(String type, Vector2f position);
		ScriptGraphNode(const ConfigNode& node);
		
		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

		void feedToHash(Hash::Hasher& hasher) override;

		void assignType(const ScriptNodeTypeCollection& nodeTypeCollection) const;
		void clearType() const;
		const IScriptNodeType& getNodeType() const;

		GraphNodePinType getPinType(GraphPinId idx) const override;
		gsl::span<const GraphNodePinType> getPinConfiguration() const override;

		OptionalLite<GraphNodeId> getParentNode() const { return parentNode; }
		void setParentNode(OptionalLite<GraphNodeId> id) { parentNode = id; }

		void offsetNodes(GraphNodeId offset) override;

	private:
		mutable const IScriptNodeType* nodeType = nullptr;
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
	
	class ScriptGraph final : public Resource, public BaseGraphImpl<ScriptGraphNode>, public std::enable_shared_from_this<ScriptGraph> {
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

		bool isPersistent() const;
		bool isSingleton() const;

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

		GraphNodeId addNode(const String& type, Vector2f pos, ConfigNode settings) override;
		void makeBaseGraph();

		OptionalLite<GraphNodeId> getStartNode() const;
		OptionalLite<GraphNodeId> getCallee(GraphNodeId node) const;
		OptionalLite<GraphNodeId> getCaller(GraphNodeId node) const;
		OptionalLite<GraphNodeId> getReturnTo(GraphNodeId node) const;
		OptionalLite<GraphNodeId> getReturnFrom(GraphNodeId node) const;
		uint64_t getHash() const;

		std::optional<GraphNodeId> getMessageInboxId(const String& messageId, bool requiresSpawningScript = false) const;

		void assignTypes(const ScriptNodeTypeCollection& nodeTypeCollection, bool force = false) const;
		void clearTypes();
		void finishGraph();

		GraphNodeId getNodeRoot(GraphNodeId nodeId) const;
		const ScriptGraphNodeRoots& getRoots() const;
		void setRoots(ScriptGraphNodeRoots roots);

		void appendGraph(GraphNodeId parent, const ScriptGraph& other);
		Vector<int> getSubGraphIndicesForAssetId(const String& id) const;
		Range<GraphNodeId> getSubGraphRange(int subGraphIdx) const;

		FunctionParameters getFunctionParameters() const;

	private:
		Vector<std::pair<GraphNodeId, GraphNodeId>> callerToCallee;
		Vector<std::pair<GraphNodeId, GraphNodeId>> returnToCaller;
		Vector<std::pair<String, Range<GraphNodeId>>> subGraphs;
		uint64_t hash = 0;

		mutable uint64_t lastAssignTypeHash = 1;

		ScriptGraphNodeRoots roots;

		GraphNodeId findNodeRoot(GraphNodeId nodeId) const;
		void generateRoots();
		[[nodiscard]] bool isMultiConnection(GraphNodePinType pinType) const override;
	};

	template <>
    class ConfigNodeSerializer<ScriptGraph> {
    public:
		ConfigNode serialize(ScriptGraph script, const EntitySerializationContext& context);
		ScriptGraph deserialize(const EntitySerializationContext& context, const ConfigNode& node);
		void deserialize(const EntitySerializationContext& context, const ConfigNode& node, ScriptGraph& target);
    };
}
