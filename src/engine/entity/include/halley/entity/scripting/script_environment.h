#pragma once
#include "../entity.h"
#include "script_graph.h"
#include "script_state.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class World;
	class ScriptEnvironment;
	class ScriptGraph;
    class ScriptState;

	enum class ScriptNodeExecutionState {
		Done,
		Executing,
		Terminate
	};
	
	class IScriptNodeType {
	public:		
        struct Result {
	        Time timeElapsed = 0;
        	ScriptNodeExecutionState state = ScriptNodeExecutionState::Done;
        };

		virtual ~IScriptNodeType() = default;

		virtual String getName() = 0;
		virtual Result update(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, IScriptStateData* curData) = 0;
		virtual std::unique_ptr<IScriptStateData> makeData() { return {}; }
        virtual void initData(IScriptStateData& data, const ScriptGraphNode& node) {}
	};

	template <typename T>
	class ScriptNodeTypeBase : public IScriptNodeType {
	public:
		static_assert(std::is_base_of_v<IScriptStateData, T>);
		
		virtual Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, T& curData) = 0;
		virtual void doInitData(T& data, const ScriptGraphNode& node) = 0;
		
		Result update(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, IScriptStateData* curData) final override { return doUpdate(environment, time, node, *dynamic_cast<T*>(curData)); }
		std::unique_ptr<IScriptStateData> makeData() override { return std::make_unique<T>(); }
		virtual void initData(IScriptStateData& data, const ScriptGraphNode& node) { doInitData(dynamic_cast<T&>(data), node); }
	};

	template <>
	class ScriptNodeTypeBase<void> : public IScriptNodeType {
	public:
		virtual Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) = 0;
		
		Result update(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, IScriptStateData*) final override { return doUpdate(environment, time, node); }
	};

    class ScriptEnvironment {
    public:
    	ScriptEnvironment(const HalleyAPI& api, World& world, Resources& resources);

    	void addBasicScriptNodes();
    	void addScriptNode(std::unique_ptr<IScriptNodeType> nodeType);

    	void update(Time time, const ScriptGraph& graph, ScriptState& state);

    	EntityRef getEntity(EntityId entityId);

    private:
		const HalleyAPI& api;
    	World& world;
    	Resources& resources;
    	std::map<String, std::unique_ptr<IScriptNodeType>> nodeTypes;
    	
        IScriptNodeType::Result updateNode(Time time, const ScriptGraphNode& node, IScriptStateData* curData);
        std::unique_ptr<IScriptStateData> makeNodeData(const ScriptGraphNode& node);
    };
}
