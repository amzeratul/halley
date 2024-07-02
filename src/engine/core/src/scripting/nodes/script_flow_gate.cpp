#include "script_flow_gate.h"

#include "halley/utils/algorithm.h"
using namespace Halley;

ScriptFlowGateData::ScriptFlowGateData(const ConfigNode& node)
{
	if (node.hasKey("flowing")) {
		flowing = node["flowing"].asBool();
	}
}

ConfigNode ScriptFlowGateData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	if (flowing) {
		result["flowing"] = *flowing;
	}
	return result;
}

gsl::span<const IScriptNodeType::PinType> ScriptFlowGate::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::FlowPin, PD::Output, true }, PinType{ ET::FlowPin, PD::Output, true } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptFlowGate::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Flow while ");
	str.append(getConnectedNodeName(node, graph, 1), parameterColour);
	return str.moveResults();
}

String ScriptFlowGate::getPinDescription(const BaseGraphNode& node, PinType element, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "Condition";
	} else if (elementIdx == 2) {
		return "Flow while true";
	} else if (elementIdx == 3) {
		return "Flow while false";
	} else {
		return ScriptNodeTypeBase<ScriptFlowGateData>::getPinDescription(node, element, elementIdx);
	}
}

void ScriptFlowGate::doInitData(ScriptFlowGateData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data.flowing.reset();
}

IScriptNodeType::Result ScriptFlowGate::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptFlowGateData& data) const
{
	const bool shouldFlow = readDataPin(environment, node, 1).asBool(false);

	if (shouldFlow != data.flowing) {
		data.flowing = shouldFlow;
		if (shouldFlow) {
			return Result(ScriptNodeExecutionState::Fork, 0, 1, 2);
		} else {
			return Result(ScriptNodeExecutionState::Fork, 0, 2, 1);
		}
	} else {
		return Result(ScriptNodeExecutionState::Executing, time);
	}
}



ScriptSwitchGateData::ScriptSwitchGateData(const ConfigNode& node)
{
	if (node.hasKey("flowing")) {
		flowing = node["flowing"].asOptional<int>();
	}
}

ConfigNode ScriptSwitchGateData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	if (flowing) {
		result["flowing"] = *flowing;
	}
	return result;	
}

Vector<IGraphNodeType::SettingType> ScriptSwitchGate::getSettingTypes() const
{
	return {
		SettingType{ "cases", "Halley::Vector<Halley::String>", Vector<String>{""} },
	};
}

gsl::span<const IGraphNodeType::PinType> ScriptSwitchGate::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 10>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output, true },
		PinType{ ET::FlowPin, PD::Output, true },
		PinType{ ET::FlowPin, PD::Output, true },
		PinType{ ET::FlowPin, PD::Output, true },
		PinType{ ET::FlowPin, PD::Output, true },
		PinType{ ET::FlowPin, PD::Output, true },
		PinType{ ET::FlowPin, PD::Output, true },
		PinType{ ET::FlowPin, PD::Output, true },
	};

	const auto cases = node.getSettings()["cases"].asVector<String>({});
	const auto numCases = std::min(cases.size(), static_cast<size_t>(7));
	return gsl::span<const PinType>(data).subspan(0, 3 + numCases);
}

std::pair<String, Vector<ColourOverride>> ScriptSwitchGate::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Switches flow based on ");
	str.append(getConnectedNodeName(node, graph, 1), parameterColour);
	return str.moveResults();
}

String ScriptSwitchGate::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx >= 2) {
		const auto cases = node.getSettings()["cases"].asVector<String>({});
		if (elementIdx - 2 >= cases.size()) {
			return "default";
		}
		return cases.at(elementIdx - 2);
	}
	return ScriptNodeTypeBase<ScriptSwitchGateData>::getPinDescription(node, elementType, elementIdx);
}

void ScriptSwitchGate::doInitData(ScriptSwitchGateData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data = ScriptSwitchGateData(nodeData);
}

IScriptNodeType::Result ScriptSwitchGate::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptSwitchGateData& data) const
{
	const auto curValue = readDataPin(environment, node, 1).asString("");
	auto cases = node.getSettings()["cases"].asVector<String>({});
	if (cases.size() > 7) {
		cases.resize(7);
	}

	int idx = 0;
	if (const auto iter = std_ex::find(cases, curValue); iter != cases.end()) {
		idx = static_cast<int>(iter - cases.begin());
	} else {
		idx = static_cast<int>(cases.size());
	}

	if (idx != data.flowing) {
		data.flowing = idx;
		const uint8_t active = 1 << idx;
		const auto allMask = static_cast<uint8_t>((1 << (cases.size() + 1)) - 1);
		return Result(ScriptNodeExecutionState::Fork, 0, active, allMask & (~active));
	} else {
		return Result(ScriptNodeExecutionState::Executing, time);
	}
}


ScriptFlowOnceData::ScriptFlowOnceData(const ConfigNode& node)
{
	if (node.hasKey("active")) {
		active = node["active"].asBool();
	}
}

ConfigNode ScriptFlowOnceData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	if (active) {
		result["active"] = *active;
	}
	return result;
}

gsl::span<const IScriptNodeType::PinType> ScriptFlowOnce::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptFlowOnce::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	const auto desc = getConnectedNodeName(node, graph, 1);
	auto str = ColourStringBuilder(true);
	str.append("Flow once");
	return str.moveResults();
}

String ScriptFlowOnce::getPinDescription(const BaseGraphNode& node, PinType element, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "Flow once";
	}
	else if (elementIdx == 2) {
		return "Flow otherwise";
	}
	else {
		return ScriptNodeTypeBase<ScriptFlowOnceData>::getPinDescription(node, element, elementIdx);
	}
}

void ScriptFlowOnce::doInitData(ScriptFlowOnceData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{

}

IScriptNodeType::Result ScriptFlowOnce::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptFlowOnceData& data) const
{
	if (!data.active) {
		data.active = true;
		return Result(ScriptNodeExecutionState::Done, 0.0);
	}
	return Result(ScriptNodeExecutionState::Done, 0, 2, 0);
}


ScriptLatchData::ScriptLatchData(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		latched = node["latched"].asBool(false);
		if (latched) {
			value = node["value"];
		}
	} else {
		latched = false;
		value = ConfigNode();
	}
}

ConfigNode ScriptLatchData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["latched"] = latched;
	if (latched) {
		result["value"] = value;
	}
	return result;
}

String ScriptLatch::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "latch(" + getConnectedNodeName(node, graph, 0) + ")";
}

gsl::span<const IScriptNodeType::PinType> ScriptLatch::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::WriteDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptLatch::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Returns ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	str.append(", or last latched value");
	return str.moveResults();
}

String ScriptLatch::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "Set Latched";
	}
	return ScriptNodeTypeBase<ScriptLatchData>::getPinDescription(node, elementType, elementIdx);
}

void ScriptLatch::doInitData(ScriptLatchData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data = ScriptLatchData(nodeData);
}

ConfigNode ScriptLatch::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptLatchData& data) const
{
	if (data.latched) {
		return ConfigNode(data.value);
	} else {
		return readDataPin(environment, node, 0);
	}
}

void ScriptLatch::doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data, ScriptLatchData& curData) const
{
	const bool newState = data.asBool(false);
	if (curData.latched != newState) {
		curData.latched = newState;
		if (newState) {
			curData.value = readDataPin(environment, node, 0);
		} else {
			curData.value = ConfigNode();
		}
	}
}


ScriptCacheData::ScriptCacheData(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		value = node["value"];
		timeElapsed = node["timeElapsed"].asFloat(0);
		lastFrame = node["lastFrame"].asInt(0);
		hasValue = node["hasValue"].asBool(false);
	}
}

ConfigNode ScriptCacheData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["value"] = value;
	result["timeElapsed"] = static_cast<float>(timeElapsed);
	result["lastFrame"] = lastFrame;
	result["hasValue"] = hasValue;
	return result;
}


Vector<IGraphNodeType::SettingType> ScriptCache::getSettingTypes() const
{
	return {
		SettingType{ "expiration", "Halley::Time", Vector<String>{"0"} },
	};
}

String ScriptCache::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "cache(" + getConnectedNodeName(node, graph, 0) + ")";
}

gsl::span<const IGraphNodeType::PinType> ScriptCache::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Output }
	};
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptCache::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Returns ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	str.append(", or cached value");
	return str.moveResults();
}

String ScriptCache::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	return ScriptNodeTypeBase<ScriptCacheData>::getPinDescription(node, elementType, elementIdx);
}

void ScriptCache::doInitData(ScriptCacheData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data = ScriptCacheData(nodeData);
}

ConfigNode ScriptCache::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptCacheData& data) const
{
	const auto curFrame = environment.getCurrentFrameNumber();

	if (data.hasValue) {
		if (data.lastFrame != curFrame) {
			data.timeElapsed += environment.getDeltaTime();
			data.lastFrame = curFrame;

			if (data.timeElapsed > node.getSettings()["expiration"].asFloat(0)) {
				data.hasValue = false;
				data.value = ConfigNode();
			}
		}
	}

	if (!data.hasValue) {
		data.value = readDataPin(environment, node, 0);
		data.hasValue = true;
		data.lastFrame = curFrame;
		data.timeElapsed = 0;
	}

	return ConfigNode(data.value);
}



ConfigNode ScriptFenceData::toConfigNode(const EntitySerializationContext& context)
{
	return ConfigNode(signaled);
}

gsl::span<const IGraphNodeType::PinType> ScriptFence::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::WriteDataPin, PD::Input }
	};
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptFence::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	return { "Waits until signaled.", {} };
}

String ScriptFence::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 2) {
		return "Signal";
	}
	return ScriptNodeTypeBase<ScriptFenceData>::getPinDescription(node, elementType, elementIdx);
}

void ScriptFence::doInitData(ScriptFenceData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	if (nodeData.getType() == ConfigNodeType::Bool) {
		data.signaled = nodeData.asBool(false);
	}
}

void ScriptFence::doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data, ScriptFenceData& curData) const
{
	curData.signaled = true;
}

IScriptNodeType::Result ScriptFence::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptFenceData& curData) const
{
	if (curData.signaled) {
		curData.signaled = false;
		return Result(ScriptNodeExecutionState::Done);
	} else {
		return Result(ScriptNodeExecutionState::Executing, time);
	}
}



ConfigNode ScriptBreakerData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["signaled"] = signaled;
	result["active"] = active;
	return result;
}

gsl::span<const IGraphNodeType::PinType> ScriptBreaker::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output, true },
		PinType{ ET::WriteDataPin, PD::Input }
	};
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptBreaker::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	return { "Flows until signaled",  {} };
}

String ScriptBreaker::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 2) {
		return "Signal";
	}
	return ScriptNodeTypeBase<ScriptBreakerData>::getPinDescription(node, elementType, elementIdx);
}

void ScriptBreaker::doInitData(ScriptBreakerData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	if (nodeData.getType() == ConfigNodeType::Undefined) {
		data.signaled = false;
		data.active = false;
	} else {
		data.signaled = nodeData["signaled"].asBool(false);
		data.active = nodeData["active"].asBool(false);
	}
}

void ScriptBreaker::doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data, ScriptBreakerData& curData) const
{
	if (curData.active) {
		curData.signaled = true;
	}
}

IScriptNodeType::Result ScriptBreaker::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptBreakerData& curData) const
{
	if (!curData.active) {
		curData.active = true;
		return Result(ScriptNodeExecutionState::ForkAndConvertToWatcher);
	} else {
		if (curData.signaled) {
			curData.signaled = false;
			curData.active = false;
			return Result(ScriptNodeExecutionState::Done, 0, 0, 1);
		} else {
			return Result(ScriptNodeExecutionState::Executing, time);
		}
	}
}



gsl::span<const IGraphNodeType::PinType> ScriptSignal::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::WriteDataPin, PD::Output }
	};
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSignal::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Signals ");
	str.append(getConnectedNodeName(node, graph, 2), parameterColour);
	return str.moveResults();
}

String ScriptSignal::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 2) {
		return "Signal";
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
}

IScriptNodeType::Result ScriptSignal::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	writeDataPin(environment, node, 2, ConfigNode(1));
	return Result(ScriptNodeExecutionState::Done);
}



ConfigNode ScriptLineResetData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["active"] = active;
	result["signaled"] = signaled;
	result["monitorVariable"] = monitorVariable;
	return result;
}

Vector<IGraphNodeType::SettingType> ScriptLineReset::getSettingTypes() const
{
	return {
		SettingType{ "flowAtStart", "bool", Vector<String>{"true"} },
	};
}

gsl::span<const IGraphNodeType::PinType> ScriptLineReset::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output, true },
		PinType{ ET::WriteDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input }
	};
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptLineReset::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Pulses output if signaled by ");
	str.append(getConnectedNodeName(node, graph, 2), parameterColour);
	str.append(" or if variable ");
	str.append(getConnectedNodeName(node, graph, 3), parameterColour);
	str.append(" changes");
	if (node.getSettings()["flowAtStart"].asBool(true)) {
		str.append(" (pulses at start)", settingColour);
	} else {
		str.append(" (wait for first change)", settingColour);
	}
	str.append(".");
	return str.moveResults();
}

String ScriptLineReset::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 2) {
		return "Signal";
	} else if (elementIdx == 3) {
		return "Monitor Variable";
	}
	return ScriptNodeTypeBase<ScriptLineResetData>::getPinDescription(node, elementType, elementIdx);
}

void ScriptLineReset::doInitData(ScriptLineResetData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data.active = false;
	data.signaled = false;
}

void ScriptLineReset::doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data, ScriptLineResetData& curData) const
{
	curData.signaled = true;
}

IScriptNodeType::Result ScriptLineReset::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptLineResetData& curData) const
{
	if (!curData.active) {
		curData.active = true;
		curData.signaled = false;
		curData.monitorVariable = environment.readInputDataPin(node, 3);

		if (node.getSettings()["flowAtStart"].asBool(true)) {
			return Result(ScriptNodeExecutionState::Fork, 0, 1);
		} else {
			return Result(ScriptNodeExecutionState::Executing, time);
		}
	} else {
		bool reset = curData.signaled;
		curData.signaled = false;

		const auto var = environment.readInputDataPin(node, 3);
		if (var != curData.monitorVariable) {
			curData.monitorVariable = var;
			reset = true;
		}

		if (reset) {
			return Result(ScriptNodeExecutionState::Fork, 0, 1, 1);
		} else {
			return Result(ScriptNodeExecutionState::Executing, time);
		}
	}
}



gsl::span<const IGraphNodeType::PinType> ScriptDetachFlow::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output, false, false, true, false }
	};
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptDetachFlow::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder();
	str.append("Continues flow in a new thread, detached from this stack");
	return str.moveResults();
}

IScriptNodeType::Result ScriptDetachFlow::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Detach);
}
