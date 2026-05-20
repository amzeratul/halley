#include "halley/input/control_binding_config.h"

using namespace Halley;

ControlBinding::ControlBinding(const ConfigNode& node)
{
	bindingType = node["type"].asEnum<ControlBindingType>();

	switch (bindingType) {
	case ControlBindingType::GamepadButton:
		gamepadButton = node["value"].asEnum<JoystickButtonPosition>();
		break;
	case ControlBindingType::GamepadAxis:
		gamepadAxis = node["value"].asEnum<JoystickAxisPosition>();
		break;
	case ControlBindingType::KeyboardButton:
		keyCode = node["value"].asEnum<KeyCode>();
		break;
	case ControlBindingType::MouseButton:
		mouseButton = node["value"].asEnum<MouseButton>();
		break;
	}
}

ConfigNode ControlBinding::toConfigNode() const
{
	ConfigNode result;

	result["type"] = bindingType;

	switch (bindingType) {
	case ControlBindingType::GamepadButton:
		result["value"] = gamepadButton;
		break;
	case ControlBindingType::GamepadAxis:
		result["value"] = gamepadAxis;
		break;
	case ControlBindingType::KeyboardButton:
		result["value"] = keyCode;
		break;
	case ControlBindingType::MouseButton:
		result["value"] = mouseButton;
		break;
	}

	return result;
}

ControlBindingType ControlBinding::getBindingType() const
{
	return bindingType;
}

void ControlBinding::bindMouseButton(MouseButton button)
{
	bindingType = ControlBindingType::MouseButton;
	mouseButton = button;
}

void ControlBinding::bindKeyboardButton(KeyCode button)
{
	bindingType = ControlBindingType::KeyboardButton;
	keyCode = button;
}

void ControlBinding::bindGamepadButton(JoystickButtonPosition button)
{
	bindingType = ControlBindingType::GamepadButton;
	gamepadButton = button;
}

void ControlBinding::bindGamepadAxis(JoystickAxisPosition axis)
{
	bindingType = ControlBindingType::GamepadAxis;
	gamepadAxis = axis;
}

void ControlBinding::unbind()
{
	bindingType = ControlBindingType::None;
}

JoystickAxisPosition ControlBinding::getJoystickAxis() const
{
	HalleyAssertDev(bindingType == ControlBindingType::GamepadAxis);
	return gamepadAxis;
}

JoystickButtonPosition ControlBinding::getJoystickButtonPosition() const
{
	HalleyAssertDev(bindingType == ControlBindingType::GamepadButton);
	return gamepadButton;
}

MouseButton ControlBinding::getMouseButton() const
{
	HalleyAssertDev(bindingType == ControlBindingType::MouseButton);
	return mouseButton;
}

KeyCode ControlBinding::getKeyCode() const
{
	HalleyAssertDev(bindingType == ControlBindingType::KeyboardButton);
	return keyCode;
}

ControlBindingConfig::ControlBindingConfig(const ConfigNode& node)
{
	bindingId = node["bindingId"].asString();
	exclusivityGroup = node["exclusivityGroup"].asString("");
	inputTypes = node["inputTypes"].asVector<InputType>({});
	defaultBindings = node["defaultBindings"].asVector<ControlBinding>({});
}

const String& ControlBindingConfig::getBindingId() const
{
	return bindingId;
}

const String& ControlBindingConfig::getExclusivityGroup() const
{
	return exclusivityGroup;
}

const Vector<InputType>& ControlBindingConfig::getInputTypes() const
{
	return inputTypes;
}

const Vector<ControlBinding>& ControlBindingConfig::getDefaultBindings() const
{
	return defaultBindings;
}

ControlBindingConfigs::ControlBindingConfigs(const ConfigNode& node)
{
	bindingConfigs = node["bindingConfigs"].asVector<ControlBindingConfig>({});
}

const Vector<ControlBindingConfig>& ControlBindingConfigs::getBindings() const
{
	return bindingConfigs;
}
