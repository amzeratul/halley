#include "halley/input/control_binding_config.h"

using namespace Halley;

std::pair<String, JoystickAxisDirection> ControlBinding::parseAxis(std::string_view axisName)
{
	auto name = axisName.substr(0, axisName.size() - 1);
	auto dir = axisName.substr(axisName.size() - 1, 1) == "+" ? JoystickAxisDirection::Positive : JoystickAxisDirection::Negative;
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

ControlBinding::ControlBinding(MouseButton button)
{
	bind(button);
}

ControlBinding::ControlBinding(KeyCode button, std::optional<KeyCode> chord)
{
	bind(button, chord);
}

ControlBinding::ControlBinding(JoystickButtonPosition button, std::optional<JoystickButtonPosition> chord)
{
	bind(button, chord);
}

ControlBinding::ControlBinding(JoystickAxisPosition axis, JoystickAxisDirection direction)
{
	bind(axis, direction);
}

namespace {
	template <typename T>
	std::pair<T, std::optional<T>> parseChord(std::string_view str)
	{
		if (const auto pos = str.find('+'); pos != std::string_view::npos) {
			const auto str0 = str.substr(0, pos);
			const auto str1 = str.substr(pos + 1);
			return { fromString<T>(str1), fromString<T>(str0) };
		} else {
			return { fromString<T>(str), std::nullopt };
		}
	}
}

void ControlBinding::loadValue(ControlBindingType type, const ConfigNode& value)
{
	switch (type) {
	case ControlBindingType::GamepadButton:
		std::tie(gamepadButton, gamepadButtonChord) = parseChord<JoystickButtonPosition>(value.asString());
		break;
	case ControlBindingType::GamepadAxis:
	{
		auto [name, dir] = parseAxis(value.asString());
		gamepadAxis = fromString<JoystickAxisPosition>(name);
		gamepadAxisDirection = dir;
		break;
	}
	case ControlBindingType::KeyboardButton:
		std::tie(keyCode, keyCodeChord) = parseChord<KeyCode>(value.asString());
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
		result["value"] = (gamepadButtonChord ? toString(*gamepadButtonChord) + "+" : String()) + gamepadButton;
		break;
	case ControlBindingType::GamepadAxis:
		result["value"] = gamepadAxis + (gamepadAxisDirection == JoystickAxisDirection::Positive ? "+" : "-");
		break;
	case ControlBindingType::KeyboardButton:
		result["value"] = (keyCodeChord ? toString(*keyCodeChord) + "+" : String()) + keyCode;
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

bool ControlBinding::bind(MouseButton button)
{
	if (bindingType != ControlBindingType::MouseButton || mouseButton != button) {
		*this = ControlBinding();
		bindingType = ControlBindingType::MouseButton;
		mouseButton = button;
		return true;
	}
	return false;
}

bool ControlBinding::bind(KeyCode button, std::optional<KeyCode> buttonChord)
{
	if (bindingType != ControlBindingType::KeyboardButton || keyCode != button || keyCodeChord != buttonChord) {
		*this = ControlBinding();
		bindingType = ControlBindingType::KeyboardButton;
		keyCode = button;
		keyCodeChord = buttonChord;
		return true;
	}
	return false;
}

bool ControlBinding::bind(JoystickButtonPosition button, std::optional<JoystickButtonPosition> buttonChord)
{
	if (bindingType != ControlBindingType::GamepadButton || gamepadButton != button || gamepadButtonChord != buttonChord) {
		*this = ControlBinding();
		bindingType = ControlBindingType::GamepadButton;
		gamepadButton = button;
		gamepadButtonChord = buttonChord;
		return true;
	}
	return false;
}

bool ControlBinding::bind(JoystickAxisPosition axis, JoystickAxisDirection direction)
{
	if (bindingType != ControlBindingType::GamepadAxis || gamepadAxis != axis || gamepadAxisDirection != direction) {
		*this = ControlBinding();
		bindingType = ControlBindingType::GamepadAxis;
		gamepadAxis = axis;
		gamepadAxisDirection = direction;
		return true;
	}
	return false;
}

bool ControlBinding::unbind()
{
	if (bindingType != ControlBindingType::None) {
		*this = ControlBinding();
		return true;
	}
	return false;
}

std::pair<JoystickAxisPosition, JoystickAxisDirection> ControlBinding::getGamepadAxis() const
{
	HalleyAssertDev(bindingType == ControlBindingType::GamepadAxis);
	HalleyAssertDev(gamepadAxis);
	HalleyAssertDev(gamepadAxisDirection);
	return { *gamepadAxis, *gamepadAxisDirection };
}

std::pair<JoystickButtonPosition, std::optional<JoystickButtonPosition>> ControlBinding::getGamepadButtonPosition() const
{
	HalleyAssertDev(bindingType == ControlBindingType::GamepadButton);
	HalleyAssertDev(gamepadButton);
	return { *gamepadButton, gamepadButtonChord };
}

MouseButton ControlBinding::getMouseButton() const
{
	HalleyAssertDev(bindingType == ControlBindingType::MouseButton);
	HalleyAssertDev(mouseButton);
	return *mouseButton;
}

std::pair<KeyCode, std::optional<KeyCode>> ControlBinding::getKeyCode() const
{
	HalleyAssertDev(bindingType == ControlBindingType::KeyboardButton);
	HalleyAssertDev(keyCode);
	return { *keyCode, keyCodeChord };
}

std::pair<int, std::optional<int>> ControlBinding::getGamepadButtonIdx(const InputDevice& gamepad) const
{
	auto [button, chord] = getGamepadButtonPosition();
	return { gamepad.getButtonAtPosition(button), chord ? std::optional(gamepad.getButtonAtPosition(*chord)) : std::nullopt };
}

std::pair<int, JoystickAxisDirection> ControlBinding::getGamepadAxisIdx(const InputDevice& gamepad) const
{
	auto [axis, dir] = getGamepadAxis();
	return { gamepad.getAxisAtPosition(axis), dir };
}

std::pair<int, std::optional<int>> ControlBinding::getKeyboardButtonIdx() const
{
	auto [button, chord] = getKeyCode();
	return { static_cast<int>(button), chord ? std::optional(static_cast<int>(*chord)) : std::nullopt };
}

int ControlBinding::getMouseButtonIdx() const
{
	return static_cast<int>(getMouseButton());
}

ControlBinding ControlBinding::convertToGamepadAxis() const
{
	if (bindingType == ControlBindingType::GamepadButton && gamepadButton && !gamepadButtonChord) {
		if (gamepadButton == JoystickButtonPosition::TriggerLeft) {
			ControlBinding result;
			result.bind(JoystickAxisPosition::TriggerLeft, JoystickAxisDirection::Positive);
			return result;
		} else if (gamepadButton == JoystickButtonPosition::TriggerRight) {
			ControlBinding result;
			result.bind(JoystickAxisPosition::TriggerRight, JoystickAxisDirection::Positive);
			return result;
		} else {
			return *this;
		}
	}
	return {};
}

ControlBindingConfig::ControlBindingConfig(const ConfigNode& node)
{
	bindingId = node["bindingId"].asString();
	bindingTargetType = bindingId.endsWith("+") || bindingId.endsWith("-") ? ControlBindingTargetType::Axis : ControlBindingTargetType::Button;
	groupId = node["group"].asString("");
	exclusivityGroup = node["exclusivityGroup"].asString("");
	inputTypes = node["inputTypes"].asVector<InputType>({});
	defaultBindings = node["defaultBindings"].asVector<ControlBinding>({});
	inheritedBindings = node["inheritedBindings"].asVector<ControlInheritedBinding>({});
	hidden = node["hidden"].asBool(false);
	devOnly = node["devOnly"].asBool(false);
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

bool ControlBindingConfig::isDevOnly() const
{
	return devOnly;
}

ControlBindingConfigs::ControlBindingConfigs(const ConfigNode& node)
{
	for (const auto& groupNode: node["bindings"]) {
		const auto group = groupNode["group"].asString("");
		const auto exclusivityGroup = groupNode["exclusivityGroup"].asString("");
		for (const auto& nOrig: groupNode["entries"]) {
			auto n = ConfigNode(nOrig);
			if (!n.hasKey("group")) {
				n["group"] = group;
			}
			if (!n.hasKey("exclusivityGroup")) {
				n["exclusivityGroup"] = exclusivityGroup;
			}
			bindingConfigs += ControlBindingConfig(n);
		}
	}

	bindingSlots = node["bindingSlots"].asVector<Vector<InputType>>({});

	for (const auto& binding: bindingConfigs) {
		bindingIds.emplace(binding.getBindingId());
	}
}

const ControlBindingConfig& ControlBindingConfigs::getBinding(const String& id) const
{
	// This might be kinda slow...
	for (const auto& b: bindingConfigs) {
		if (b.getBindingId() == id) {
			return b;
		}
	}
	throw Exception("Binding not found: " + id, 0);
}

const Vector<ControlBindingConfig>& ControlBindingConfigs::getBindings() const
{
	return bindingConfigs;
}

const Vector<Vector<InputType>>& ControlBindingConfigs::getBindingSlots() const
{
	return bindingSlots;
}

const HashSet<String>& ControlBindingConfigs::getBindingIds() const
{
	return bindingIds;
}
