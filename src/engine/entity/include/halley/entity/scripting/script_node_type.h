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

		virtual std::vector<SettingType> getSettingTypes() const;
		virtual std::pair<String, std::vector<ColourOverride>> getDescription(const ScriptGraphNode& node, const World& world, PinType elementType, uint8_t elementIdx) const;
		virtual std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world) const;
		virtual std::pair<String, std::vector<ColourOverride>> getPinDescription(const ScriptGraphNode& node, PinType elementType, uint8_t elementIdx) const;
		virtual String getIconName() const = 0;
		virtual ScriptNodeClassification getClassification() const = 0;
		
		virtual gsl::span<const PinType> getPinConfiguration() const = 0;
        PinType getPin(size_t n) const;

		virtual bool canAdd() const { return true; }
        virtual bool canDelete() const { return true; }
		
		virtual std::unique_ptr<IScriptStateData> makeData() const { return {}; }
        virtual void initData(IScriptStateData& data, const ScriptGraphNode& node) const {}

		virtual Result update(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, IScriptStateData* curData) const = 0;
		virtual ConfigNode getData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) = 0;
		virtual void setData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) = 0;
	};

	template <typename DataType, typename EnvironmentType = ScriptEnvironment>
	class ScriptNodeTypeBase : public IScriptNodeType {
	public:
		static_assert(std::is_base_of_v<IScriptStateData, DataType>);
		static_assert(std::is_base_of_v<ScriptEnvironment, EnvironmentType>);
		
		virtual Result doUpdate(EnvironmentType& environment, Time time, const ScriptGraphNode& node, DataType& curData) const = 0;
		virtual void doInitData(DataType& data, const ScriptGraphNode& node) const = 0;
		virtual ConfigNode doGetData(EnvironmentType& environment, const ScriptGraphNode& node, size_t pinN) { return ConfigNode(); }
		virtual void doSetData(EnvironmentType& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) {}
		
		std::unique_ptr<IScriptStateData> makeData() const override { return std::make_unique<DataType>(); }
		virtual void initData(IScriptStateData& data, const ScriptGraphNode& node) const { doInitData(dynamic_cast<DataType&>(data), node); }

		Result update(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, IScriptStateData* curData) const final override { return doUpdate(dynamic_cast<EnvironmentType&>(environment), time, node, *dynamic_cast<DataType*>(curData)); }
		ConfigNode getData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) final override { return doGetData(dynamic_cast<EnvironmentType&>(environment), node, pinN); }
		void setData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) final override { doSetData(dynamic_cast<EnvironmentType&>(environment), node, pinN, std::move(data)); }
	};

	template <typename EnvironmentType>
	class ScriptNodeTypeBase<void, EnvironmentType> : public IScriptNodeType {
	public:
		static_assert(std::is_base_of_v<ScriptEnvironment, EnvironmentType>);
		
		virtual Result doUpdate(EnvironmentType& environment, Time time, const ScriptGraphNode& node) const { return Result(ScriptNodeExecutionState::Done); }
		virtual ConfigNode doGetData(EnvironmentType& environment, const ScriptGraphNode& node, size_t pinN) { return ConfigNode(); }
		virtual void doSetData(EnvironmentType& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) {}

		Result update(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, IScriptStateData*) const final override { return doUpdate(dynamic_cast<EnvironmentType&>(environment), time, node); }
		ConfigNode getData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) final override { return doGetData(dynamic_cast<EnvironmentType&>(environment), node, pinN); }
		void setData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) final override { doSetData(dynamic_cast<EnvironmentType&>(environment), node, pinN, std::move(data)); }
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
