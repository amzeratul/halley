#pragma once
#include "../entity.h"
#include "script_graph.h"
#include "script_state.h"
#include "halley/core/graphics/text/text_renderer.h"
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

	enum class ScriptNodeClassification {
		Terminator, // As in start/end, not as in Arnie
		FlowControl,
		Condition,
		Action
	};
	
	class IScriptNodeType {
	public:		
        struct Result {
	        Time timeElapsed = 0;
        	ScriptNodeExecutionState state = ScriptNodeExecutionState::Done;
        };

		virtual ~IScriptNodeType() = default;

		virtual String getName() const = 0;

		virtual std::pair<String, std::vector<ColourOverride>> getDescription(const ScriptGraphNode& node, const World& world) const = 0;
		virtual String getIconName() const = 0;
		virtual ScriptNodeClassification getClassification() const = 0;
		
		virtual uint8_t getNumInputPins() const { return 1; }
		virtual uint8_t getNumOutputPins() const { return 1; }
		virtual uint8_t getNumTargetPins() const { return 0; }
		virtual bool hasSettings() const { return false; }
		
		virtual Result update(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, IScriptStateData* curData) const = 0;
		virtual std::unique_ptr<IScriptStateData> makeData() const { return {}; }
        virtual void initData(IScriptStateData& data, const ScriptGraphNode& node) const {}
	};

	template <typename T>
	class ScriptNodeTypeBase : public IScriptNodeType {
	public:
		static_assert(std::is_base_of_v<IScriptStateData, T>);
		
		virtual Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, T& curData) const = 0;
		virtual void doInitData(T& data, const ScriptGraphNode& node) const = 0;
		
		Result update(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, IScriptStateData* curData) const final override { return doUpdate(environment, time, node, *dynamic_cast<T*>(curData)); }
		std::unique_ptr<IScriptStateData> makeData() const override { return std::make_unique<T>(); }
		virtual void initData(IScriptStateData& data, const ScriptGraphNode& node) const { doInitData(dynamic_cast<T&>(data), node); }
	};

	template <>
	class ScriptNodeTypeBase<void> : public IScriptNodeType {
	public:
		virtual Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const = 0;
		
		Result update(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, IScriptStateData*) const final override { return doUpdate(environment, time, node); }
	};

	class ScriptNodeTypeCollection {
	public:
		ScriptNodeTypeCollection();
    	void addScriptNode(std::unique_ptr<IScriptNodeType> nodeType);

		const IScriptNodeType* tryGetNodeType(const String& typeId) const;
		
    private:
    	std::map<String, std::unique_ptr<IScriptNodeType>> nodeTypes;

		void addBasicScriptNodes();
	};
}
