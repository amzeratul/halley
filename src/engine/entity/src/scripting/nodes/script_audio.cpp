#include "script_audio.h"
using namespace Halley;

Vector<IScriptNodeType::SettingType> ScriptAudioEvent::getSettingTypes() const
{
	return {
		SettingType{ "event", "Halley::ResourceReference<Halley::AudioEvent>", Vector<String>{""} },
		SettingType{ "destroyEvent", "Halley::ResourceReference<Halley::AudioEvent>", Vector<String>{""} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptAudioEvent::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptAudioEvent::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	const auto& event = node.getSettings()["event"].asString("");
	const auto& destroyEvent = node.getSettings()["destroyEvent"].asString("");

	auto str = ColourStringBuilder(true);
	str.append("Post audio event ");
	str.append(event, parameterColour);
	str.append(" on entity ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	if (!destroyEvent.isEmpty()) {
		str.append(", then post audio event ");
		str.append(destroyEvent, parameterColour);
		str.append(" on destroy");
	}
	return str.moveResults();
}

bool ScriptAudioEvent::hasDestructor(const ScriptGraphNode& node) const
{
	return !node.getSettings()["destroyEvent"].asString("").isEmpty();
}

IScriptNodeType::Result ScriptAudioEvent::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	environment.postAudioEvent(node.getSettings()["event"].asString(""), readEntityId(environment, node, 2));
	return Result(ScriptNodeExecutionState::Done);
}

void ScriptAudioEvent::doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node) const
{
	environment.postAudioEvent(node.getSettings()["destroyEvent"].asString(""), readEntityId(environment, node, 2));
}

