#include "script_ui.h"

#include "../../../../ui/include/halley/ui/ui_widget.h"
using namespace Halley;

Vector<IScriptNodeType::SettingType> ScriptUIModal::getSettingTypes() const
{
	return {
		SettingType{ "ui", "Halley::ResourceReference<Halley::UIDefinition>", Vector<String>{""} }
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptUIModal::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptUIModal::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Open modal UI ");
	str.append(node.getSettings()["ui"].asString(""), parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptUIModal::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	// TODO
	return Result(ScriptNodeExecutionState::Done);
}



ConfigNode ScriptUIInWorldData::toConfigNode(const EntitySerializationContext& context)
{
	// TODO
	return ConfigNode();
}

Vector<IScriptNodeType::SettingType> ScriptUIInWorld::getSettingTypes() const
{
	return {
		SettingType{ "ui", "Halley::ResourceReference<Halley::UIDefinition>", Vector<String>{""} },
		SettingType{ "alignment", "Halley::Vector2f", Vector<String>{"0.5", "0.5"}},
		SettingType{ "offset", "Halley::Vector2f", Vector<String>{"0", "0"}}
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptUIInWorld::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptUIInWorld::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Open UI ");
	str.append(node.getSettings()["ui"].asString(""), parameterColour);
	str.append(" on entity ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	str.append(" with alignment ");
	str.append(toString(node.getSettings()["alignment"].asVector2f(Vector2f(0.5f, 0.5f))), parameterColour);
	str.append(" and offset ");
	str.append(toString(node.getSettings()["offset"].asVector2f(Vector2f())), parameterColour);
	return str.moveResults();
}

void ScriptUIInWorld::doInitData(ScriptUIInWorldData& data, const ScriptGraphNode& node, const EntitySerializationContext& context,	const ConfigNode& nodeData) const
{
	data.ui.reset();
}

IScriptNodeType::Result ScriptUIInWorld::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptUIInWorldData& data) const
{
	const String ui = node.getSettings()["ui"].asString("");
	const EntityId entityId = readEntityId(environment, node, 2);
	const auto alignment = node.getSettings()["alignment"].asVector2f(Vector2f(0.5f, 0.5f));
	const auto offset = node.getSettings()["offset"].asVector2f(Vector2f());

	data.ui = environment.createInWorldUI(ui, offset, alignment, entityId);

	return Result(ScriptNodeExecutionState::Done);
}

void ScriptUIInWorld::doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptUIInWorldData& data) const
{
	if (data.ui) {
		data.ui->destroy();
		data.ui.reset();
	}
}
