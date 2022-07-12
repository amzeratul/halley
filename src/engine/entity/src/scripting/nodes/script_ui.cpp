#include "script_ui.h"

#include "../../../../ui/include/halley/ui/ui_widget.h"
using namespace Halley;

ConfigNode ScriptUIModalData::toConfigNode(const EntitySerializationContext& context)
{
	return ConfigNode(result);
}

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
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType { ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptUIModal::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Open modal UI ");
	str.append(node.getSettings()["ui"].asString(""), parameterColour);
	str.append(", wait for it, then output result value");
	return str.moveResults();
}

String ScriptUIModal::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId element_idx) const
{
	return "ui.value";
}

void ScriptUIModal::doInitData(ScriptUIModalData& data, const ScriptGraphNode& node, const EntitySerializationContext& context,	const ConfigNode& nodeData) const
{
	data.ui.reset();
	data.result = nodeData;
}

IScriptNodeType::Result ScriptUIModal::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptUIModalData& data) const
{
	if (!data.ui) {
		const String ui = node.getSettings()["ui"].asString("");
		data.ui = environment.createModalUI(ui);
	}
	if (data.ui->isAlive()) {
		return Result(ScriptNodeExecutionState::Executing, time);
	} else {
		data.result = data.ui->getResultValue();
		data.ui.reset();
		return Result(ScriptNodeExecutionState::Done);
	}
}

void ScriptUIModal::doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptUIModalData& data) const
{
	if (data.ui) {
		data.ui->destroy();
		data.ui.reset();
		data.result = ConfigNode();
	}
}

ConfigNode ScriptUIModal::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pin_n, ScriptUIModalData& data) const
{
	return ConfigNode(data.result);
}


ConfigNode ScriptUIInWorldData::toConfigNode(const EntitySerializationContext& context)
{
	// TODO?
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
	if (!data.ui) {
		const String ui = node.getSettings()["ui"].asString("");
		const EntityId entityId = readEntityId(environment, node, 2);
		const auto alignment = node.getSettings()["alignment"].asVector2f(Vector2f(0.5f, 0.5f));
		const auto offset = node.getSettings()["offset"].asVector2f(Vector2f());

		data.ui = environment.createInWorldUI(ui, offset, alignment, entityId);
		return Result(ScriptNodeExecutionState::Fork);
	}

	return Result(ScriptNodeExecutionState::Executing, time);
}

void ScriptUIInWorld::doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptUIInWorldData& data) const
{
	if (data.ui) {
		data.ui->destroy();
		data.ui.reset();
	}
}
