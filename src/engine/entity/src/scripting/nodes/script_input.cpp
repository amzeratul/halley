#include "script_input.h"

#include "halley/core/input/input_device.h"
using namespace Halley;

ConfigNode ScriptInputButtonData::toConfigNode(const EntitySerializationContext& context)
{
	return ConfigNode();
}

String ScriptInputButton::getLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["button"].asString("");
}

Vector<IScriptNodeType::SettingType> ScriptInputButton::getSettingTypes() const
{
	return {
		SettingType{ "button", "Halley::InputButton", Vector<String>{"primary"} },
		SettingType{ "device", "int", Vector<String>{"0"} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptInputButton::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 5>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::FlowPin, PD::Output, true }, PinType{ ET::FlowPin, PD::Output, true } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptInputButton::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Input ");
	str.append(node.getSettings()["button"].asString(""), parameterColour);
	str.append(" on ");
	str.append(toString(node.getSettings()["device"].asInt(0)), parameterColour);
	return str.moveResults();
}

std::pair<String, Vector<ColourOverride>> ScriptInputButton::getPinDescription(const ScriptGraphNode& node, PinType element, ScriptPinId elementIdx) const
{
	switch (elementIdx) {
	case 1:
		return { "Flow Output when button is pressed", {}};
	case 2:
		return { "Flow Output when button is released", {}};
	case 3:
		return { "Flow Output while button is held", {}};
	case 4:
		return { "Flow Output while button is not held", {}};
	default:
		return ScriptNodeTypeBase<ScriptInputButtonData>::getPinDescription(node, element, elementIdx);
	}
}

void ScriptInputButton::doInitData(ScriptInputButtonData& data, const ScriptGraphNode& node, const EntitySerializationContext& context,	const ConfigNode& nodeData) const
{
	data.initialised = false;
}

IScriptNodeType::Result ScriptInputButton::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptInputButtonData& data) const
{
	const int device = node.getSettings()["device"].asInt(0);
	const int button = environment.getInputButtonByName(node.getSettings()["button"].asString("primary"));
	const auto input = environment.getInputDevice(device);
	if (input) {
		const bool firstRun = !data.initialised;
		data.initialised = true;

		const bool pressed = input->isButtonPressed(button) || (firstRun && input->isButtonDown(button));
		const bool released = input->isButtonReleased(button) || (firstRun && !input->isButtonDown(button));

		if (pressed || released) {
			constexpr uint8_t pressedPin = 1;
			constexpr uint8_t releasedPin = 2;
			constexpr uint8_t heldPin = 4;
			constexpr uint8_t notHeldPin = 8;

			const uint8_t activate = (pressed ? (pressedPin | heldPin) : 0) | (released ? (releasedPin | notHeldPin) : 0);
			const uint8_t cancel = (pressed ? notHeldPin : 0) | (released ? heldPin : 0);

			return Result(ScriptNodeExecutionState::Fork, time, activate, cancel);
		}
	}

	return Result(ScriptNodeExecutionState::Executing, time);
}
