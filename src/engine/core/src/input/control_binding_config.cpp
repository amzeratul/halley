#include "halley/input/control_binding_config.h"

using namespace Halley;

std::pair<String, ControlBindingAxisDirection> ControlBinding::parseAxis(std::string_view axisName)
{
	auto name = axisName.substr(0, axisName.size() - 1);
	auto dir = axisName.substr(axisName.size() - 1, 1) == "+" ? ControlBindingAxisDirection::Positive : ControlBindingAxisDirection::Negative;
	return { name, dir };
}

ControlBinding::ControlBinding(const ConfigNode& node)
{
	if (node.hasKey("type")) {
		bindingType = node["type"].asEnum<ControlBindingType>();
		loadValue(bindingType, node["value"]);
	} else {
		const auto& m = node.asMap();
		if (m.size() == 1) {
			for (auto& [k, v]: m) {
				bindingType = fromString<ControlBindingType>(k);
				loadValue(bindingType, v);
			}
		}
	}
}

void ControlBinding::loadValue(ControlBindingType type, const ConfigNode& value)
{
	switch (type) {
	case ControlBindingType::GamepadButton:
		gamepadButton = value.asEnum<JoystickButtonPosition>();
		break;
	case ControlBindingType::GamepadAxis:
	{
		auto [name, dir] = parseAxis(value.asString());
		gamepadAxis = fromString<JoystickAxisPosition>(name);
		gamepadAxisDirection = dir;
		break;
	}
	case ControlBindingType::KeyboardButton:
		keyCode = value.asEnum<KeyCode>();
		break;
	case ControlBindingType::MouseButton:
		mouseButton = value.asEnum<MouseButton>();
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
		result["value"] = gamepadAxis + (gamepadAxisDirection == ControlBindingAxisDirection::Positive ? "+" : "-");
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

ControlInheritedBinding::ControlInheritedBinding(const ConfigNode& node)
{
	const auto& m = node.asMap();
	if (m.size() == 1) {
		for (auto& [k, v]: m) {
			bindingType = fromString<ControlBindingType>(k);
			sourceId = v.asString("");
		}
	}
}

ControlBindingType ControlInheritedBinding::getBindingType() const
{
	return bindingType;
}

const String& ControlInheritedBinding::getSourceId() const
{
	return sourceId;
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

void ControlBinding::bindGamepadAxis(JoystickAxisPosition axis, ControlBindingAxisDirection direction)
{
	bindingType = ControlBindingType::GamepadAxis;
	gamepadAxis = axis;
	gamepadAxisDirection = direction;
}

void ControlBinding::unbind()
{
	bindingType = ControlBindingType::None;
}

std::pair<JoystickAxisPosition, ControlBindingAxisDirection> ControlBinding::getJoystickAxis() const
{
	HalleyAssertDev(bindingType == ControlBindingType::GamepadAxis);
	return { gamepadAxis, gamepadAxisDirection };
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
	bindingTargetType = bindingId.endsWith("+") || bindingId.endsWith("-") ? ControlBindingTargetType::Axis : ControlBindingTargetType::Button;
	groupId = node["groupId"].asString("");
	exclusivityGroup = node["exclusivityGroup"].asString("");
	inputTypes = node["inputTypes"].asVector<InputType>({});
	defaultBindings = node["defaultBindings"].asVector<ControlBinding>({});
	inheritedBindings = node["inheritedBindings"].asVector<ControlInheritedBinding>({});
	hidden = node["hidden"].asBool(false);
}

const String& ControlBindingConfig::getBindingId() const
{
	return bindingId;
}

ControlBindingTargetType ControlBindingConfig::getTargetType() const
{
	return bindingTargetType;
}

const String& ControlBindingConfig::getGroupId() const
{
	return groupId;
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

const Vector<ControlInheritedBinding>& ControlBindingConfig::getInheritedBindings() const
{
	return inheritedBindings;
}

bool ControlBindingConfig::isHidden() const
{
	return hidden;
}

ControlBindingConfigs::ControlBindingConfigs(const ConfigNode& node)
{
	for (const auto& groupNode: node["bindings"]) {
		const auto group = groupNode["group"].asString("");
		const auto exclusivityGroup = groupNode["exclusivityGroup"].asString("");
		for (const auto& nOrig: groupNode["entries"]) {
			auto n = ConfigNode(nOrig);
			n["group"] = group;
			n["exclusivityGroup"] = exclusivityGroup;
			bindingConfigs += ControlBindingConfig(n);
		}
	}

	bindingSlots = node["bindingSlots"].asVector<Vector<InputType>>({});
}

const Vector<ControlBindingConfig>& ControlBindingConfigs::getBindings() const
{
	return bindingConfigs;
}

const Vector<Vector<InputType>>& ControlBindingConfigs::getBindingSlots() const
{
	return bindingSlots;
}
