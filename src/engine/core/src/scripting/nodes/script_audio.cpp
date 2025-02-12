#include "script_audio.h"
using namespace Halley;

ScriptAudioEventData::ScriptAudioEventData(const ConfigNode& node)
{
	active = node.asBool(false);
}

ConfigNode ScriptAudioEventData::toConfigNode(const EntitySerializationContext& context)
{
	return ConfigNode(active);
}

Vector<IScriptNodeType::SettingType> ScriptAudioEvent::getSettingTypes() const
{
	return {
		SettingType{ "event", "Halley::ResourceReference<Halley::AudioEvent>", Vector<String>{""} },
		SettingType{ "destroyEvent", "Halley::ResourceReference<Halley::AudioEvent>", Vector<String>{""} },
		SettingType{ "variables", "Halley::Vector<Halley::String>", Vector<String>{""} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptAudioEvent::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 10>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input },
	};

	const auto nPins = 5 + node.getSettings()["variables"].asVector<String>({}).size();
	return gsl::span<const PinType>(data).subspan(0, nPins);
}

std::pair<String, Vector<ColourOverride>> ScriptAudioEvent::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	const auto& event = tryGetConnectedNodeName(node, graph, 3).value_or(node.getSettings()["event"].asString(""));
	const auto& destroyEvent = tryGetConnectedNodeName(node, graph, 4).value_or(node.getSettings()["destroyEvent"].asString(""));

	auto str = ColourStringBuilder(true);
	str.append("Post audio event ");
	str.append(event, settingColour);
	str.append(" on entity ");
	str.append(getConnectedNodeName(node, graph, 2), parameterColour);
	if (!destroyEvent.isEmpty()) {
		str.append(", then post audio event ");
		str.append(destroyEvent, settingColour);
		str.append(" on destroy");
	}
	return str.moveResults();
}

String ScriptAudioEvent::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 3) {
		return "Event";
	} else if (elementIdx == 4) {
		return "Destroy Event";
	} else if (elementIdx >= 5) {
		auto variableNames = node.getSettings()["variables"].asVector<String>({});
		return variableNames.at(elementIdx - 5);
	}
	return ScriptNodeTypeBase<ScriptAudioEventData>::getPinDescription(node, elementType, elementIdx);
}

bool ScriptAudioEvent::hasDestructor(const ScriptGraphNode& node) const
{
	return true;
	//return !node.getSettings()["destroyEvent"].asString("").isEmpty();
}

void ScriptAudioEvent::doInitData(ScriptAudioEventData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data = ScriptAudioEventData(nodeData);
}

IScriptNodeType::Result ScriptAudioEvent::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptAudioEventData& data) const
{
	const auto entityId = readEntityId(environment, node, 2);

	auto variableNames = node.getSettings()["variables"].asVector<String>({});
	for (size_t i = 0; i < variableNames.size(); ++i) {
		const auto value = readDataPin(environment, node, i + 5).asFloat(0);
		environment.getInterface<IAudioSystemInterface>().setVariable(entityId, variableNames[i], value);
	}

	if (data.active) {
		return Result(ScriptNodeExecutionState::Executing, time);
	} else {
		auto event = readDataPin(environment, node, 3).asString(node.getSettings()["event"].asString(""));
		environment.postAudioEvent(event, entityId);

		if (variableNames.empty()) {
			return Result(ScriptNodeExecutionState::Done);
		} else {
			data.active = true;
			return Result(ScriptNodeExecutionState::ForkAndConvertToWatcher);
		}
	}
}

void ScriptAudioEvent::doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptAudioEventData& data) const
{
	data.active = false;
	auto destroyEvent = readDataPin(environment, node, 4).asString(node.getSettings()["destroyEvent"].asString(""));
	environment.postAudioEvent(destroyEvent, readEntityId(environment, node, 2));
}

