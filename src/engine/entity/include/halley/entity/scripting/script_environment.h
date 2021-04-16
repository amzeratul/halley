#pragma once
#include "script_graph.h"
#include "script_state.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class World;
	class ScriptEnvironment;
	class ScriptGraph;
    class ScriptState;

	class IScriptNodeType {
	public:
        struct Result {
	        Time timeElapsed = 0;
        	bool done = true;
        };

		virtual ~IScriptNodeType() = default;

		virtual String getName() = 0;
		virtual Result update(ScriptEnvironment& environment, Time time, const ConfigNode& settings, IScriptStateData* curData) = 0;
		virtual std::unique_ptr<IScriptStateData> makeData() { return {}; }
	};

	template <typename T>
	class ScriptNodeTypeBase : public IScriptNodeType {
	public:
		static_assert(std::is_base_of_v<IScriptNodeType, T>);
		
		virtual Result doUpdate(ScriptEnvironment& environment, Time time, const ConfigNode& settings, T& curData) = 0;
		
		Result update(ScriptEnvironment& environment, Time time, const ConfigNode& settings, IScriptStateData* curData) final override { return doUpdate(environment, time, settings, dynamic_cast<T*>(curData)); }
		std::unique_ptr<IScriptStateData> makeData() override { return std::make_unique<T>(); }
	};

	template <>
	class ScriptNodeTypeBase<void> : public IScriptNodeType {
	public:
		virtual Result doUpdate(ScriptEnvironment& environment, Time time, const ConfigNode& settings) = 0;
		
		Result update(ScriptEnvironment& environment, Time time, const ConfigNode& settings, IScriptStateData*) final override { return doUpdate(environment, time, settings); }
	};

    class ScriptEnvironment {
    public:
    	ScriptEnvironment(const HalleyAPI& api, World& world, Resources& resources);

    	void addBasicScriptNodes();
    	void addScriptNode(std::unique_ptr<IScriptNodeType> nodeType);

    	void update(Time time, const ScriptGraph& graph, ScriptState& state);

    private:
		const HalleyAPI& api;
    	World& world;
    	Resources& resources;
    	std::map<String, std::unique_ptr<IScriptNodeType>> nodeTypes;
    	
        IScriptNodeType::Result updateNode(Time time, const ScriptGraphNode& node, IScriptStateData* curData);
        std::unique_ptr<IScriptStateData> makeNodeData(const String& type);
    };
}
