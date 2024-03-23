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
	const static auto data = std::array<PinType, 8>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input },
	};

	const auto nPins = 3 + node.getSettings()["variables"].asVector<String>({}).size();
	return gsl::span<const PinType>(data).subspan(0, nPins);
}

std::pair<String, Vector<ColourOverride>> ScriptAudioEvent::getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const
{
	const auto& event = node.getSettings()["event"].asString("");
	const auto& destroyEvent = node.getSettings()["destroyEvent"].asString("");

	auto str = ColourStringBuilder(true);
	str.append("Post audio event ");
	str.append(event, settingColour);
	str.append(" on entity ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	if (!destroyEvent.isEmpty()) {
		str.append(", then post audio event ");
		str.append(destroyEvent, settingColour);
		str.append(" on destroy");
	}
	return str.moveResults();
}

String ScriptAudioEvent::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx >= 3) {
		auto variableNames = node.getSettings()["variables"].asVector<String>({});
		return variableNames.at(elementIdx - 3);
	}
	return ScriptNodeTypeBase<ScriptAudioEventData>::getPinDescription(node, elementType, elementIdx);
}

bool ScriptAudioEvent::hasDestructor(const ScriptGraphNode& node) const
{
	return !node.getSettings()["destroyEvent"].asString("").isEmpty();
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
		const auto value = readDataPin(environment, node, i + 3).asFloat(0);
		environment.getInterface<IAudioSystemInterface>().setVariable(entityId, variableNames[i], value);
	}

	if (data.active) {
		return Result(ScriptNodeExecutionState::Executing, time);
	} else {
		environment.postAudioEvent(node.getSettings()["event"].asString(""), entityId);

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
	environment.postAudioEvent(node.getSettings()["destroyEvent"].asString(""), readEntityId(environment, node, 2));
}

