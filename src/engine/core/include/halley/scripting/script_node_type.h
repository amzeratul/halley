#pragma once
#include "halley/entity/entity.h"
#include "script_graph.h"
#include "script_state.h"
#include "script_node_enums.h"
#include "halley/graph/base_graph_type.h"
#include "halley/graphics/text/text_renderer.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class World;
	class ScriptEnvironment;
	class ScriptGraph;
    class ScriptState;
	class BaseGraphNode;
	
	class IScriptNodeType : public IGraphNodeType {
	public:
        struct Result {
        	ScriptNodeExecutionState state = ScriptNodeExecutionState::Done;
        	uint8_t outputsActive = 1;
			uint8_t outputsCancelled = 0;
			GraphNodeId nodeRef = 0;
	        Time timeElapsed = 0;

        	Result() = default;
        	Result(ScriptNodeExecutionState state, Time timeElapsed = 0, uint8_t outputsActive = 1, uint8_t outputsCancelled = 0, GraphNodeId nodeRef = 0)
        		: state(state), outputsActive(outputsActive), outputsCancelled(outputsCancelled), nodeRef(nodeRef), timeElapsed(timeElapsed)
        	{}
        };

		virtual ~IScriptNodeType() = default;

		virtual ScriptNodeClassification getClassification() const = 0;

		std::pair<String, Vector<ColourOverride>> getDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx, const BaseGraph& graph) const override;
		std::pair<String, Vector<ColourOverride>> getPinAndConnectionDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx, const BaseGraph& graph) const;
		virtual String getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const;
		virtual String getLargeLabel(const BaseGraphNode& node) const;

        virtual bool canKeepData() const { return false; }
		virtual bool hasDestructor(const ScriptGraphNode& node) const { return false; }
		virtual bool showDestructor() const { return true; }

		virtual std::unique_ptr<IScriptStateData> makeData() const { return {}; }
        virtual void initData(IScriptStateData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const {}

		virtual Result update(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, IScriptStateData* curData) const = 0;
		virtual void destructor(ScriptEnvironment& environment, const ScriptGraphNode& node, IScriptStateData* curData) const = 0;
		virtual bool isStackRollbackPoint(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId outPin, IScriptStateData* curData) const = 0;

		virtual ConfigNode getData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, IScriptStateData* curData) const = 0;
		virtual void setData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data, IScriptStateData* curData) const = 0;
        virtual EntityId getEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN, IScriptStateData* curData) const = 0;
		virtual ConfigNode getDevConData(ScriptEnvironment& environment, const ScriptGraphNode& node, IScriptStateData* curData) const = 0;

		template <typename Env>
		FORCEINLINE static ConfigNode readDataPin(Env& environment, const ScriptGraphNode& node, size_t pinN)
		{
			return environment.readInputDataPin(node, static_cast<GraphPinId>(pinN));
		}

		void writeDataPin(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const;
		EntityId readEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t idx) const;
		EntityId readRawEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t idx) const;
		std::optional<String> tryGetConnectedNodeName(const BaseGraphNode& node, const BaseGraph& graph, size_t pinN) const;
		String getConnectedNodeName(const BaseGraphNode& node, const BaseGraph& graph, size_t pinN) const;

		String getPinTypeName(PinType type) const override;

		struct OutputNode {
			OptionalLite<GraphNodeId> dstNode;
			GraphPinId outputPin;
			GraphPinId inputPin;
		};
		std::array<OutputNode, 8> getOutputNodes(const ScriptGraphNode& node, uint8_t outputActiveMask) const;
		GraphPinId getNthOutputPinIdx(const ScriptGraphNode& node, size_t n) const;

        static String addParentheses(String str);

		Colour4f getColour() const override;
		int getSortOrder() const override;
	};

	template <typename DataType>
	class ScriptNodeTypeBase : public IScriptNodeType {
	public:
		static_assert(std::is_base_of_v<IScriptStateData, DataType>);
		
		virtual Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, DataType& curData) const { return Result(ScriptNodeExecutionState::Done); }
		virtual void doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, DataType& curData) const {}
		virtual bool doIsStackRollbackPoint(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId outPin, DataType& curData) const { return false; }
		virtual void doInitData(DataType& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const = 0;
		virtual ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, DataType& curData) const { return ConfigNode(); }
		virtual void doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data, DataType& curData) const {}
		virtual EntityId doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN, DataType& curData) const { return EntityId(); }
		virtual ConfigNode doGetDevConData(ScriptEnvironment& environment, const ScriptGraphNode& node, DataType& curData) const { return {}; }

		std::unique_ptr<IScriptStateData> makeData() const override { return std::make_unique<DataType>(); }
		void initData(IScriptStateData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override { doInitData(dynamic_cast<DataType&>(data), node, context, nodeData); }

		FORCEINLINE Result update(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, IScriptStateData* curData) const final override { return doUpdate(environment, time, node, *dynamic_cast<DataType*>(curData)); }
		FORCEINLINE void destructor(ScriptEnvironment& environment, const ScriptGraphNode& node, IScriptStateData* curData) const final override { return doDestructor(environment, node, *dynamic_cast<DataType*>(curData)); }
		FORCEINLINE bool isStackRollbackPoint(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId outPin, IScriptStateData* curData) const final override { return doIsStackRollbackPoint(environment, node, outPin, *dynamic_cast<DataType*>(curData)); }
		FORCEINLINE ConfigNode getData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, IScriptStateData* curData) const final override { return doGetData(environment, node, pinN, *dynamic_cast<DataType*>(curData)); }
		FORCEINLINE void setData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data, IScriptStateData* curData) const final override { doSetData(environment, node, pinN, std::move(data), *dynamic_cast<DataType*>(curData)); }
		FORCEINLINE EntityId getEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN, IScriptStateData* curData) const final override { return doGetEntityId(environment, node, pinN, *dynamic_cast<DataType*>(curData)); }
		FORCEINLINE ConfigNode getDevConData(ScriptEnvironment& environment, const ScriptGraphNode& node, IScriptStateData* curData) const override { return doGetDevConData(environment, node, *dynamic_cast<DataType*>(curData)); }
	};

	template <>
	class ScriptNodeTypeBase<void> : public IScriptNodeType {
	public:
		virtual Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const { return Result(ScriptNodeExecutionState::Done); }
		virtual void doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node) const {}
		virtual bool doIsStackRollbackPoint(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId outPin) const { return false; }
		virtual ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const { return ConfigNode(); }
		virtual void doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const {}
		virtual EntityId doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const { return EntityId(); }
		virtual ConfigNode doGetDevConData(ScriptEnvironment& environment, const ScriptGraphNode& node) const { return {}; }

		FORCEINLINE Result update(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, IScriptStateData*) const final override { return doUpdate(environment, time, node); }
		FORCEINLINE void destructor(ScriptEnvironment& environment, const ScriptGraphNode& node, IScriptStateData*) const final override { return doDestructor(environment, node); }
		FORCEINLINE bool isStackRollbackPoint(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId outPin, IScriptStateData*) const final override { return doIsStackRollbackPoint(environment, node, outPin); }
		FORCEINLINE ConfigNode getData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, IScriptStateData*) const final override { return doGetData(environment, node, pinN); }
		FORCEINLINE void setData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data, IScriptStateData*) const final override { doSetData(environment, node, pinN, std::move(data)); }
		FORCEINLINE EntityId getEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN, IScriptStateData*) const final override { return doGetEntityId(environment, node, pinN); }
		FORCEINLINE ConfigNode getDevConData(ScriptEnvironment& environment, const ScriptGraphNode& node, IScriptStateData*) const override { return doGetDevConData(environment, node); }
	};

	class ScriptNodeTypeCollection : public GraphNodeTypeCollection {
	public:
		ScriptNodeTypeCollection();

    	void addScriptNode(std::unique_ptr<IGraphNodeType> nodeType);
		const IScriptNodeType* tryGetNodeType(const String& typeId) const;

	private:
		void addBasicScriptNodes();
	};
}
