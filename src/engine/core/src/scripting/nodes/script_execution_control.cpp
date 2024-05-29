#include "script_execution_control.h"

using namespace Halley;

Vector<IScriptNodeType::SettingType> ScriptStart::getSettingTypes() const
{
	return {
		SettingType{ "dataPins", "Halley::Vector<Halley::String>", Vector<String>{""} },
		SettingType{ "targetPins", "Halley::Vector<Halley::String>", Vector<String>{""} },
	};
}

std::pair<String, Vector<ColourOverride>> ScriptStart::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	return { "Start script", {} };
}

gsl::span<const IScriptNodeType::PinType> ScriptStart::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	
	const size_t nDataOutput = node.getSettings()["dataPins"].getSequenceSize();
	const size_t nTargetOutput = node.getSettings()["targetPins"].getSequenceSize();

	if (nDataOutput > 4 || nTargetOutput > 0) {
		static thread_local std::vector<PinType> pins;
		pins.clear();
		pins.reserve(16);

		pins.emplace_back(ET::FlowPin, PD::Output);
		for (size_t i = 0; i < nDataOutput; ++i) {
			pins.emplace_back(ET::ReadDataPin, PD::Output);
		}
		for (size_t i = 0; i < nTargetOutput; ++i) {
			pins.emplace_back(ET::TargetPin, PD::Output);
		}

		return pins;
	} else {
		// Simple, common case
		using ET = ScriptNodeElementType;
		using PD = GraphNodePinDirection;
		const static auto data = std::array<PinType, 5>{ PinType{ ET::FlowPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output } };
		return gsl::span<const PinType>(data).subspan(0, 1 + nDataOutput);
	}
}

String ScriptStart::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	const size_t nOutput = 1;
	const size_t nDataInput = node.getSettings()["dataPins"].getSequenceSize(0);
	const size_t nTargetInput = node.getSettings()["targetPins"].getSequenceSize(0);

	size_t idx = 0;
	const char* key = nullptr;

	if (elementIdx < nOutput) {
		// Do nothing, let it fall back
	} else if (elementIdx < nOutput + nDataInput) {
		idx = elementIdx - nOutput;
		key = "dataPins";
	} else if (elementIdx < nOutput + nDataInput + nTargetInput) {
		idx = elementIdx - nOutput - nDataInput;
		key = "targetPins";
	}

	if (key) {
		auto name = node.getSettings()[key].asSequence().at(idx).asString();
		if (!name.isEmpty()) {
			return name;
		}
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
}

String ScriptStart::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return getPinDescription(node, ScriptNodeElementType::ReadDataPin, elementIdx);
}

IScriptNodeType::Result ScriptStart::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Done);
}

ConfigNode ScriptStart::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	if (const auto other = getOtherPin(environment, node, pinN)) {
		const auto& returnNode = environment.getCurrentGraph()->getNodes()[other->first];
		return environment.readInputDataPin(returnNode, other->second);
	} else {
		return ConfigNode(environment.getStartParams()[pinN - 1]);
	}
}

EntityId ScriptStart::doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const
{
	if (const auto other = getOtherPin(environment, node, pinN)) {
		const auto& returnNode = environment.getCurrentGraph()->getNodes()[other->first];
		return environment.readInputEntityId(returnNode, other->second);
	} else {
		return EntityId(environment.getStartParams()[pinN - 1].asEntityIdHolder().value);
	}
}

std::optional<std::pair<GraphNodeId, GraphPinId>> ScriptStart::getOtherPin(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	// Find the "call" node
	const auto& graph = environment.getCurrentGraph();
	const auto callerNodeId = graph->getCaller(node.getId());
	if (!callerNodeId) {
		return std::nullopt;
	}

	return std::pair<GraphNodeId, GraphPinId>{ *callerNodeId, static_cast<GraphPinId>(pinN) };
}


std::pair<String, Vector<ColourOverride>> ScriptDestructor::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	return { "On destruction", {} };
}

gsl::span<const IScriptNodeType::PinType> ScriptDestructor::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::FlowPin, PD::Output } };
	return data;
}

IScriptNodeType::Result ScriptDestructor::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Done);
}



gsl::span<const IScriptNodeType::PinType> ScriptRestart::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::FlowPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptRestart::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	return { "Restart script", {} };
}

IScriptNodeType::Result ScriptRestart::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Restart);
}



gsl::span<const IScriptNodeType::PinType> ScriptStop::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::FlowPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptStop::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	return { "Terminate script", {} };
}

IScriptNodeType::Result ScriptStop::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Terminate);
}



gsl::span<const IScriptNodeType::PinType> ScriptSpinwait::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::FlowPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSpinwait::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	return { "Spinwait thread", {} };
}

IScriptNodeType::Result ScriptSpinwait::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Executing, time);
}

ConfigNode ScriptStartScriptData::toConfigNode(const EntitySerializationContext& context)
{
	auto result = ConfigNode::MapType();
	result["scriptName"] = scriptName;
	result["target"] = EntityIdHolder{ target.value };
	return result;
}


Vector<IScriptNodeType::SettingType> ScriptStartScript::getSettingTypes() const
{
	return {
		SettingType{ "script", "Halley::ResourceReference<Halley::ScriptGraph>", Vector<String>{""} },
		SettingType{ "tags", "Halley::Vector<Halley::String>", Vector<String>{""} },
		SettingType{ "terminateOnDestruction", "bool", Vector<String>{"false"} },
		SettingType{ "terminateAllThreadsOnDestruction", "bool", Vector<String>{"false"} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptStartScript::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;

	auto& settings = node.getSettings();

	const size_t nDataInput = settings["nDataInput"].asInt(0);
	const size_t nTargetInput = settings["nTargetInput"].asInt(0);

	static thread_local std::vector<PinType> pins;
	pins.clear();
	pins.reserve(16);

	pins.emplace_back(ET::FlowPin, PD::Input);
	pins.emplace_back(ET::FlowPin, PD::Output);
	pins.emplace_back(ET::TargetPin, PD::Input);
	pins.emplace_back(ET::ReadDataPin, PD::Input);

	for (size_t i = 0; i < nDataInput; ++i) {
		pins.emplace_back(ET::ReadDataPin, PD::Input);
	}
	for (size_t i = 0; i < nTargetInput; ++i) {
		pins.emplace_back(ET::TargetPin, PD::Input);
	}

	return pins;
}

void ScriptStartScript::updateSettings(BaseGraphNode& node, const BaseGraph& graph, Resources& resources) const
{
	auto& settings = node.getSettings();
	const auto& scriptName = settings["script"].asString("");
	if (scriptName.isEmpty() || !resources.exists<ScriptGraph>(scriptName)) {
		settings["nDataInput"] = 0;
		settings["nTargetInput"] = 0;
		settings.removeKey("inputNames");
	} else {
		const auto script = resources.get<ScriptGraph>(scriptName);
		const auto pars = script->getFunctionParameters();
		settings["nDataInput"] = pars.nDataInput;
		settings["nTargetInput"] = pars.nTargetInput;
		settings["inputNames"] = pars.inputNames;
	}
}

std::pair<String, Vector<ColourOverride>> ScriptStartScript::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	const auto scriptName = getConnectedNodeName(node, graph, 3);

	auto str = ColourStringBuilder(true);
	str.append("Start Script ");
	if (scriptName == "<empty>") {
		str.append(node.getSettings()["script"].asString(""), settingColour);
	} else {
		str.append(scriptName, parameterColour);
	}
	str.append(" on ");
	str.append(getConnectedNodeName(node, graph, 2), parameterColour);
	str.append(" with tags ");
	str.append(node.getSettings()["tags"].asString("{}"), settingColour);
	if (hasDestructor(dynamic_cast<const ScriptGraphNode&>(node))) {
		str.append(" and ");
		if (node.getSettings()["terminateAllThreadsOnDestruction"].asBool(false)) {
			str.append("terminate all threads", parameterColour);
		} else {
			str.append("terminate main thread", parameterColour);
		}
		str.append(" on destruction");
	}
	return str.moveResults();
}

String ScriptStartScript::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx < 4) {
		if (elementIdx == 2) {
			return "Script Target";
		} else if (elementIdx == 3) {
			return "Script Name Override";
		} else {
			return ScriptNodeTypeBase<ScriptStartScriptData>::getPinDescription(node, elementType, elementIdx);
		}
	} else {
		return node.getSettings()["inputNames"].asSequence()[elementIdx - 4].asString("");
	}
}

bool ScriptStartScript::hasDestructor(const ScriptGraphNode& node) const
{
	return node.getSettings()["terminateOnDestruction"].asBool(false);
}

void ScriptStartScript::doInitData(ScriptStartScriptData& data, const ScriptGraphNode& node, const EntitySerializationContext& context,	const ConfigNode& nodeData) const
{
	if (nodeData.getType() == ConfigNodeType::Map) {
		data.scriptName = nodeData["scriptName"].asString("");
		data.target = EntityId(nodeData["target"].asEntityIdHolder({}).value);
	} else {
		data.scriptName = {};
		data.target = {};
	}
}

IScriptNodeType::Result ScriptStartScript::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node,	ScriptStartScriptData& data) const
{
	const auto script = environment.readInputDataPin(node, 3).asString(node.getSettings()["script"].asString(""));
	const auto& tags = node.getSettings()["tags"].asVector<String>({});
	const auto target = readEntityId(environment, node, 2);

	if (!script.isEmpty()) {
		Vector<ConfigNode> params;
		const size_t nDataInput = node.getSettings()["nDataInput"].asInt(0);
		const size_t nTargetInput = node.getSettings()["nTargetInput"].asInt(0);
		for (size_t i = 0; i < nDataInput; ++i) {
			params.push_back(readDataPin(environment, node, 4 + i));
		}
		for (size_t i = 0; i < nTargetInput; ++i) {
			params.push_back(ConfigNode(EntityIdHolder{ readEntityId(environment, node, 4 + nDataInput + i).value } ));
		}

		environment.startScript(target, script, tags, params);
		data.scriptName = script;
		data.target = target;
	}

	return Result(ScriptNodeExecutionState::Done);
}

void ScriptStartScript::doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptStartScriptData& data) const
{
	if (hasDestructor(node) && data.target.isValid() && !data.scriptName.isEmpty()) {
		environment.stopScript(data.target, data.scriptName, node.getSettings()["terminateAllThreadsOnDestruction"].asBool(false));
	}
}

Vector<IScriptNodeType::SettingType> ScriptStopScript::getSettingTypes() const
{
	return {
		SettingType{ "script", "Halley::ResourceReference<Halley::ScriptGraph>", Vector<String>{""} },
	};
}

std::pair<String, Vector<ColourOverride>> ScriptStopScript::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Stop Script ");
	str.append(node.getSettings()["script"].asString(""), settingColour);
	str.append(" on ");
	str.append(getConnectedNodeName(node, graph, 2), parameterColour);
	return str.moveResults();
}

gsl::span<const IScriptNodeType::PinType> ScriptStopScript::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input } };
	return data;
}

IScriptNodeType::Result ScriptStopScript::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto& script = node.getSettings()["script"].asString("");
	const auto target = readEntityId(environment, node, 2);

	if (!script.isEmpty()) {
		environment.stopScript(target, script);
	}

	return Result(ScriptNodeExecutionState::Done);
}



Vector<IScriptNodeType::SettingType> ScriptStopTag::getSettingTypes() const
{
	return {
		SettingType{ "tag", "Halley::String", Vector<String>{""} },
	};
}

std::pair<String, Vector<ColourOverride>> ScriptStopTag::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Stop all Scripts matching tag ");
	str.append(node.getSettings()["tag"].asString(""), settingColour);
	str.append(" on ");
	str.append(getConnectedNodeName(node, graph, 2), parameterColour);
	return str.moveResults();
}

gsl::span<const IScriptNodeType::PinType> ScriptStopTag::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input } };
	return data;
}

IScriptNodeType::Result ScriptStopTag::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto& tag = node.getSettings()["tag"].asString("");
	const auto target = readEntityId(environment, node, 2);

	if (!tag.isEmpty()) {
		environment.stopScriptTag(target, tag);
	}

	return Result(ScriptNodeExecutionState::Done);
}


ConfigNode ScriptWaitUntilEndOfFrameData::toConfigNode(const EntitySerializationContext& context)
{
	if (lastFrame) {
		return ConfigNode(*lastFrame);
	}
	return {};
}


Vector<IScriptNodeType::SettingType> ScriptWaitUntilEndOfFrame::getSettingTypes() const
{
	return { };
}

gsl::span<const IScriptNodeType::PinType> ScriptWaitUntilEndOfFrame::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptWaitUntilEndOfFrame::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Wait until one frame has passed and then continue");
	return str.moveResults();
}

void ScriptWaitUntilEndOfFrame::doInitData(ScriptWaitUntilEndOfFrameData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data.lastFrame = nodeData.asOptional<int>();
}

IScriptNodeType::Result ScriptWaitUntilEndOfFrame::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptWaitUntilEndOfFrameData& data) const
{
	if (!data.lastFrame) {
		data.lastFrame = environment.getCurrentFrameNumber();
	} else {
		if (data.lastFrame != environment.getCurrentFrameNumber()) {
			return Result(ScriptNodeExecutionState::Done);
		}
	}

	return Result(ScriptNodeExecutionState::Executing, time);
}