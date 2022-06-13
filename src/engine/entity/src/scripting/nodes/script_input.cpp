#include "script_input.h"

#include "halley/core/input/input_device.h"
using namespace Halley;

Vector<IScriptNodeType::SettingType> ScriptInputButton::getSettingTypes() const
{
	return {
		SettingType{ "button", "int", Vector<String>{"0"} },
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
	str.append("Monitors input button ");
	str.append(toString(node.getSettings()["button"].asInt(0)), Colour4f(0.97f, 0.35f, 0.35f));
	str.append(" on device ");
	str.append(toString(node.getSettings()["device"].asInt(0)), Colour4f(0.97f, 0.35f, 0.35f));
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
		return ScriptNodeTypeBase<void>::getPinDescription(node, element, elementIdx);
	}
}

IScriptNodeType::Result ScriptInputButton::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const int device = node.getSettings()["device"].asInt(0);
	const int button = node.getSettings()["button"].asInt(0);
	const auto input = environment.getInputDevice(device);
	if (input) {
		const bool pressed = input->isButtonPressed(button);
		const bool released = input->isButtonReleased(button);

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
