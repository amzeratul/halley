#include "halley/input/control_binding_config.h"

#include "halley/utils/hash.h"

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

ControlBinding::ControlBinding(KeyCode button, KeyMods mods)
{
	bind(button, mods);
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
		std::tie(keyCode, keyMods) = KeyCodes::fromStringWithMods(value.asString());
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
		result["value"] = (gamepadButtonChord ? toString(*gamepadButtonChord) + "+" : String()) + *gamepadButton;
		break;
	case ControlBindingType::GamepadAxis:
		result["value"] = *gamepadAxis + (gamepadAxisDirection == JoystickAxisDirection::Positive ? "+" : "-");
		break;
	case ControlBindingType::KeyboardButton:
		result["value"] = KeyCodes::toString(*keyCode, *keyMods);
		break;
	case ControlBindingType::MouseButton:
		result["value"] = *mouseButton;
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

InputType ControlBinding::getBindingInputType() const
{
	switch (bindingType) {
	case ControlBindingType::GamepadButton:
	case ControlBindingType::GamepadAxis:
		return InputType::Gamepad;
	case ControlBindingType::KeyboardButton:
		return InputType::Keyboard;
	case ControlBindingType::MouseButton:
		return InputType::Mouse;
	case ControlBindingType::None:
		return InputType::None;
	}
	return InputType::None;
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

bool ControlBinding::bind(KeyCode button, KeyMods mods)
{
	if (bindingType != ControlBindingType::KeyboardButton || keyCode != button || keyMods != mods) {
		*this = ControlBinding();
		bindingType = ControlBindingType::KeyboardButton;
		keyCode = button;
		keyMods = mods;
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
	return { *gamepadButton, gamepadButtonChord.to_optional() };
}

MouseButton ControlBinding::getMouseButton() const
{
	HalleyAssertDev(bindingType == ControlBindingType::MouseButton);
	HalleyAssertDev(mouseButton);
	return *mouseButton;
}

std::pair<KeyCode, KeyMods> ControlBinding::getKeyCode() const
{
	HalleyAssertDev(bindingType == ControlBindingType::KeyboardButton);
	HalleyAssertDev(keyCode);
	return { *keyCode, *keyMods };
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
	auto [button, mods] = getKeyCode();
	std::optional<int> chord;

	if (mods == KeyMods::Ctrl) {
		chord = static_cast<int>(KeyCode::LCtrl);
	} else if (mods == KeyMods::Shift) {
		chord = static_cast<int>(KeyCode::LShift);
	} else if (mods == KeyMods::Alt) {
		chord = static_cast<int>(KeyCode::LAlt);
	} else if (mods == KeyMods::Mod) {
		chord = static_cast<int>(KeyCode::LGUI);
	}

	return { static_cast<int>(button), chord };
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

uint64_t ControlBinding::getHash() const
{
	uint64_t result = 0;
	char* dst = reinterpret_cast<char*>(&result);
	size_t pos = 0;
	auto feed = [&] (const auto& v) {
		memcpy(dst + pos, &v, sizeof(v));
		pos += sizeof(v);
	};

	feed(bindingType);
	if (bindingType == ControlBindingType::GamepadButton) {
		feed(gamepadButton);
		feed(gamepadButtonChord);
	} else if (bindingType == ControlBindingType::GamepadAxis) {
		feed(gamepadAxis);
		feed(gamepadAxisDirection);
	} else if (bindingType == ControlBindingType::KeyboardButton) {
		feed(keyCode);
		feed(keyMods);
	} else if (bindingType == ControlBindingType::MouseButton) {
		feed(mouseButton);
	}

	return result;
}

void ControlBinding::feedToHash(Hash::Hasher& hasher) const
{
	hasher.feed(getHash());
}

ControlBindingConfig::ControlBindingConfig(const ConfigNode& node)
{
	bindingId = node["bindingId"].asString();
	bindingTargetType = bindingId.endsWith("+") || bindingId.endsWith("-") ? ControlBindingTargetType::Axis : ControlBindingTargetType::Button;
	groupId = node["group"].asString("");
	exclusivityGroup = node["exclusivityGroup"].asString("");
	canOverlap = node["canOverlap"].asVector<String>({});
	inputTypes = node["inputTypes"].asVector<InputType>({});
	defaultBindings = node["defaultBindings"].asVector<ControlBinding>({});
	inheritedBindings = node["inheritedBindings"].asVector<ControlInheritedBinding>({});
	hidden = node["hidden"].asBool(false);
	devOnly = node["devOnly"].asBool(false);
	optional = node["optional"].asBool(false);
	critical = node["critical"].asBool(false);
	toggle = node["toggle"].asBool(false);
	alwaysBindKeyMod = node["alwaysBindKeyMod"].asBool(false);
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

const Vector<String>& ControlBindingConfig::getCanOverlap() const
{
	return canOverlap;
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

bool ControlBindingConfig::isOptional() const
{
	return optional;
}

bool ControlBindingConfig::isCritical() const
{
	return critical;
}

bool ControlBindingConfig::isToggle() const
{
	return toggle;
}

bool ControlBindingConfig::isAlwaysBindKeyMod() const
{
	return alwaysBindKeyMod;
}

bool ControlBindingConfig::requiresInputType(InputType input) const
{
	return !optional && inputTypes.contains(input);
}

bool ControlBindingConfig::hasRelevantBindings(const Vector<InputType>& types) const
{
	for (const auto& t: inputTypes) {
		if (types.contains(t)) {
			return true;
		}
	}
	for (const auto& b: defaultBindings) {
		if (types.contains(b.getBindingInputType())) {
			return true;
		}
	}
	for (const auto& b: inheritedBindings) {
		// TODO
		//if (types.contains(b.getBindingType())) {
		//	return true;
		//}
	}
	return false;
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
	exclusionGroups = node["exclusionGroups"].asVector<Vector<String>>({});

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

bool ControlBindingConfigs::areBindingsMutuallyExclusive(const ControlBindingConfig& a, const ControlBindingConfig& b) const
{
	HalleyAssertDev(a.getBindingId() != b.getBindingId());

	if (a.getCanOverlap().contains(b.getBindingId()) || b.getCanOverlap().contains(a.getBindingId())) {
		return false;
	}

	const auto& groupA = a.getExclusivityGroup();
	const auto& groupB = b.getExclusivityGroup();
	if (groupA == groupB) {
		return true;
	}

	for (const auto& group: exclusionGroups) {
		if (group.contains(groupA) && group.contains(groupB)) {
			return true;
		}
	}

	return false;
}
