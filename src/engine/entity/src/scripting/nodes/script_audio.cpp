#include "script_audio.h"
using namespace Halley;

Vector<IScriptNodeType::SettingType> ScriptAudioEvent::getSettingTypes() const
{
	return { SettingType{ "event", "Halley::String", Vector<String>{""} } };
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
	auto str = ColourStringBuilder(true);
	str.append("Post audio event ");
	str.append(node.getSettings()["event"].asString(""), Colour4f(0.97f, 0.35f, 0.35f));
	str.append(" on entity ");
	str.append(getConnectedNodeName(world, node, graph, 2), Colour4f(0.97f, 0.35f, 0.35f));
	return str.moveResults();
}

IScriptNodeType::Result ScriptAudioEvent::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	environment.postAudioEvent(node.getSettings()["event"].asString(""), readEntityId(environment, node, 2));
	return Result(ScriptNodeExecutionState::Done);
}

