#pragma once
#include "../entity.h"
#include "script_graph.h"
#include "script_state.h"
#include "script_node_enums.h"
#include "halley/core/graphics/text/text_renderer.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class World;
	class ScriptEnvironment;
	class ScriptGraph;
    class ScriptState;
	
	class IScriptNodeType {
	public:		
        struct Result {
        	ScriptNodeExecutionState state = ScriptNodeExecutionState::Done;
        	uint32_t outputsActive = 1;
	        Time timeElapsed = 0;

        	Result() = default;
        	Result(ScriptNodeExecutionState state, Time timeElapsed = 0, uint32_t outputsActive = 1)
        		: state(state), outputsActive(outputsActive), timeElapsed(timeElapsed)
        	{}
        };

		struct SettingType {
			String name;
			String type;
			std::vector<String> defaultValue;
		};

		using PinType = ScriptNodePinType;

		virtual ~IScriptNodeType() = default;

		virtual String getId() const = 0;
		virtual String getName() const = 0;
		virtual String getShortDescription(const World& world, const ScriptGraphNode& node, const ScriptGraph& graph) const;
		virtual String getLabel(const ScriptGraphNode& node) const;

		virtual std::vector<SettingType> getSettingTypes() const;
		virtual std::pair<String, std::vector<ColourOverride>> getDescription(const ScriptGraphNode& node, const World& world, PinType elementType, uint8_t elementIdx, const ScriptGraph& graph) const;
		virtual std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const;
		virtual std::pair<String, std::vector<ColourOverride>> getPinDescription(const ScriptGraphNode& node, PinType elementType, uint8_t elementIdx) const;
		virtual String getIconName(const ScriptGraphNode& node) const = 0;
		virtual ScriptNodeClassification getClassification() const = 0;
		
		virtual gsl::span<const PinType> getPinConfiguration() const = 0;
        PinType getPin(size_t n) const;

		virtual bool canAdd() const { return true; }
        virtual bool canDelete() const { return true; }
		
		virtual std::unique_ptr<IScriptStateData> makeData() const { return {}; }
        virtual void initData(IScriptStateData& data, const ScriptGraphNode& node, const ConfigNode& nodeData) const {}

		virtual Result update(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, IScriptStateData* curData) const = 0;
		virtual ConfigNode getData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const = 0;
		virtual void setData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const = 0;
        virtual EntityId getEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, uint8_t pinN) const = 0;

		ConfigNode readDataPin(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const;
		void writeDataPin(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const;
		EntityId readEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t idx) const;
		String getConnectedNodeName(const World& world, const ScriptGraphNode& node, const ScriptGraph& graph, size_t pinN) const;

		std::array<OptionalLite<uint32_t>, 8> getOutputNodes(const ScriptGraphNode& node, uint32_t outputActiveMask) const;

		static String addParentheses(String str);
	};

	template <typename DataType, typename EnvironmentType = ScriptEnvironment>
	class ScriptNodeTypeBase : public IScriptNodeType {
	public:
		static_assert(std::is_base_of_v<IScriptStateData, DataType>);
		static_assert(std::is_base_of_v<ScriptEnvironment, EnvironmentType>);
		
		virtual Result doUpdate(EnvironmentType& environment, Time time, const ScriptGraphNode& node, DataType& curData) const = 0;
		virtual void doInitData(DataType& data, const ScriptGraphNode& node, const ConfigNode& nodeData) const = 0;
		virtual ConfigNode doGetData(EnvironmentType& environment, const ScriptGraphNode& node, size_t pinN) const { return ConfigNode(); }
		virtual void doSetData(EnvironmentType& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const {}
		virtual EntityId doGetEntityId(EnvironmentType& environment, const ScriptGraphNode& node, uint8_t pinN) const { return EntityId(); }
		
		std::unique_ptr<IScriptStateData> makeData() const override { return std::make_unique<DataType>(); }
		virtual void initData(IScriptStateData& data, const ScriptGraphNode& node, const ConfigNode& nodeData) const { doInitData(dynamic_cast<DataType&>(data), node, nodeData); }

		Result update(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, IScriptStateData* curData) const final override { return doUpdate(dynamic_cast<EnvironmentType&>(environment), time, node, *dynamic_cast<DataType*>(curData)); }
		ConfigNode getData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const final override { return doGetData(dynamic_cast<EnvironmentType&>(environment), node, pinN); }
		void setData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const final override { doSetData(dynamic_cast<EnvironmentType&>(environment), node, pinN, std::move(data)); }
		EntityId getEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, uint8_t pinN) const final override { return doGetEntityId(dynamic_cast<EnvironmentType&>(environment), node, pinN); }
	};

	template <typename EnvironmentType>
	class ScriptNodeTypeBase<void, EnvironmentType> : public IScriptNodeType {
	public:
		static_assert(std::is_base_of_v<ScriptEnvironment, EnvironmentType>);
		
		virtual Result doUpdate(EnvironmentType& environment, Time time, const ScriptGraphNode& node) const { return Result(ScriptNodeExecutionState::Done); }
		virtual ConfigNode doGetData(EnvironmentType& environment, const ScriptGraphNode& node, size_t pinN) const { return ConfigNode(); }
		virtual void doSetData(EnvironmentType& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const {}
		virtual EntityId doGetEntityId(EnvironmentType& environment, const ScriptGraphNode& node, uint8_t pinN) const { return EntityId(); }

		Result update(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, IScriptStateData*) const final override { return doUpdate(dynamic_cast<EnvironmentType&>(environment), time, node); }
		ConfigNode getData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const final override { return doGetData(dynamic_cast<EnvironmentType&>(environment), node, pinN); }
		void setData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const final override { doSetData(dynamic_cast<EnvironmentType&>(environment), node, pinN, std::move(data)); }
		EntityId getEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, uint8_t pinN) const final override { return doGetEntityId(dynamic_cast<EnvironmentType&>(environment), node, pinN); }
	};

	class ScriptNodeTypeCollection {
	public:
		ScriptNodeTypeCollection();
    	void addScriptNode(std::unique_ptr<IScriptNodeType> nodeType);

		const IScriptNodeType* tryGetNodeType(const String& typeId) const;
		std::vector<String> getTypes(bool includeNonAddable) const;
		std::vector<String> getNames(bool includeNonAddable) const;

	private:
    	std::map<String, std::unique_ptr<IScriptNodeType>> nodeTypes;

		void addBasicScriptNodes();
	};
}
