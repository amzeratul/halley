#include "halley/input/control_bindings.h"

#include "../support/StackWalker/StackWalker.h"
#include "halley/api/input_api.h"
#include "halley/input/input_virtual.h"
#include "halley/utils/algorithm.h"
#include "halley/input/input_keyboard.h"

using namespace Halley;

ControlBindings::ControlBindings(ControlBindingConfigs config)
	: config(std::move(config))
{
	resolve(true);
}

void ControlBindings::load(const ConfigNode& node)
{
	userBindings = node["userBindings"].asHashMap<String, ControlBinding>();
	clearRedundantBindings();
	resolve(true);
}

ConfigNode ControlBindings::toConfigNode() const
{
	ConfigNode result;
	result["userBindings"] = userBindings;
	return result;
}

const ControlBindingConfigs& ControlBindings::getConfig() const
{
	return config;
}

std::optional<size_t> ControlBindings::findSlotIdx(const Vector<ControlBinding>& bs, ControlBindingType type) const
{
	InputType inputType = InputType::None;
	switch (type) {
	case ControlBindingType::GamepadAxis:
	case ControlBindingType::GamepadButton:
		inputType = InputType::Gamepad;
		break;
	case ControlBindingType::KeyboardButton:
		inputType = InputType::Keyboard;
		break;
	case ControlBindingType::MouseButton:
		inputType = InputType::Mouse;
	}

	std::optional<size_t> bestResult;
	size_t bestIdx = std::numeric_limits<size_t>::max();

	const auto& slots = config.getBindingSlots();
	for (size_t i = 0; i < slots.size(); ++i) {
		if (bs[i].getBindingType() == ControlBindingType::None) {
			const auto idx = std_ex::find_index(slots[i], inputType);
			if (idx && *idx < bestIdx) {
				bestResult = i;
				bestIdx = *idx;
			}
		}
	}

	return bestResult;
}

Vector<ControlBinding> ControlBindings::getDefaultBindings(const ControlBindingConfig& bindingConfig) const
{
	Vector<ControlBinding> dst;
	dst.resize(config.getBindingSlots().size());

	for (const auto& defBinding: bindingConfig.getDefaultBindings()) {
		if (auto idx = findSlotIdx(dst, defBinding.getBindingType())) {
			dst[*idx] = defBinding;
		} else {
			Logger::logError("Unable to bind default controls for " + bindingConfig.getBindingId());
		}
	}

	return dst;
}

void ControlBindings::resetToDefaults()
{
	if (!userBindings.empty()) {
		userBindings.clear();

		modified = true;
		++version;
		resolve(true);
	}
}

void ControlBindings::resetToDefaults(gsl::span<int> slots)
{
	bool mod = false;
	for (auto slot: slots) {
		const String suffix = String(":") + slot;
		mod = std_ex::erase_if_key(userBindings, [&] (const String& k) { return k.endsWith(suffix); }) || mod;
	}

	if (mod) {
		modified = true;
		++version;
		resolve(true);
	}
}

bool ControlBindings::hasChanges() const
{
	return !userBindings.empty();
}

void ControlBindings::resolve(bool force) const
{
	if (!modified && !force) {
		return;
	}

	resolvedBindings.clear();

	// Bind defaults
	for (const auto& bindingConfig: config.getBindings()) {
		resolvedBindings[bindingConfig.getBindingId()] = getDefaultBindings(bindingConfig);
	}

	// Bind user
	for (const auto& [key, binding]: userBindings) {
		const auto split = key.split(':');
		const auto id = split[0];
		const auto slot = split[1].toInteger();

		if (const auto iter = resolvedBindings.find(id); iter != resolvedBindings.end()) {
			auto& dst = iter->second;
			if (slot >= 0 && slot < static_cast<int>(dst.size())) {
				dst[slot] = binding;
			} else {
				Logger::logError("Unable to bind user control for " + id + " at slot " + slot + ": slot not found");
			}
		} else {
			Logger::logError("Unable to bind user control for " + id + " at slot " + slot + ": id not found");
		}
	}

	// Bind inherited
	for (const auto& bindingConfig: config.getBindings()) {
		auto& dst = resolvedBindings[bindingConfig.getBindingId()];

		for (const auto& inhBinding: bindingConfig.getInheritedBindings()) {
			if (auto idx = findSlotIdx(dst, inhBinding.getBindingType())) {
				const auto& src = resolvedBindings.at(inhBinding.getSourceId());
				for (const auto& srcBinding: src) {
					if (inhBinding.getBindingType() == srcBinding.getBindingType()) {
						dst[*idx] = srcBinding;
					} else if (inhBinding.getBindingType() == ControlBindingType::GamepadAxis && srcBinding.getBindingType() == ControlBindingType::GamepadButton) {
						dst[*idx] = srcBinding.convertToGamepadAxis();
					}
				}
			} else {
				Logger::logError("Unable to bind inherited controls for " + bindingConfig.getBindingId());
			}
		}
	}

	modified = false;
}

void ControlBindings::clearRedundantBindings()
{
	Vector<String> toErase;

	for (const auto& [key, binding]: userBindings) {
		const auto split = key.split(':');
		const auto id = split[0];
		const auto slot = split[1].toInteger();

		if (getDefaultBindings(config.getBinding(id))[slot] == binding) {
			toErase += key;
		}
	}

	if (!toErase.empty()) {
		std_ex::erase_if_key(userBindings, [&] (const String& key) { return toErase.contains(key); });
		modified = true;
		++version;
	}
}

bool ControlBindings::bindKeyboard(std::string_view bindingId, size_t slot, KeyCode code)
{
	if (auto* binding = tryGetUserBinding(bindingId, slot)) {
		if (binding->bind(code)) {
			clearRedundantBindings();
			modified = true;
			++version;
			return true;
		}
	}
	return false;
}

bool ControlBindings::bindGamepadButton(std::string_view bindingId, size_t slot, JoystickButtonPosition button)
{
	if (auto* binding = tryGetUserBinding(bindingId, slot)) {
		if (binding->bind(button)) {
			clearRedundantBindings();
			modified = true;
			++version;
			return true;
		}
	}
	return false;
}

bool ControlBindings::bindGamepadAxis(std::string_view bindingId, size_t slot, JoystickAxisPosition axis, JoystickAxisDirection dir)
{
	if (auto* binding = tryGetUserBinding(bindingId, slot)) {
		if (binding->bind(axis, dir)) {
			clearRedundantBindings();
			modified = true;
			++version;
			return true;
		}
	}
	return false;
}

bool ControlBindings::bindMouseButton(std::string_view bindingId, size_t slot, MouseButton button)
{
	if (auto* binding = tryGetUserBinding(bindingId, slot)) {
		if (binding->bind(button)) {
			clearRedundantBindings();
			modified = true;
			++version;
			return true;
		}
	}
	return false;
}

bool ControlBindings::bind(std::string_view bindingId, size_t slot, ControlBinding newBinding)
{
	if (auto* binding = tryGetUserBinding(bindingId, slot)) {
		if (*binding != newBinding) {
			*binding = newBinding;
			clearRedundantBindings();
			modified = true;
			++version;
			return true;
		}
	}
	return false;
}

bool ControlBindings::unbind(std::string_view bindingId, size_t slot)
{
	if (auto* binding = tryGetUserBinding(bindingId, slot)) {
		if (binding->unbind()) {
			clearRedundantBindings();
			modified = true;
			++version;
			return true;
		}
	}
	return false;
}

const Vector<ControlBinding>& ControlBindings::getBindings(std::string_view bindingId) const
{
	resolve(resolvedBindings.empty());
	if (const auto iter = resolvedBindings.find(bindingId); iter != resolvedBindings.end()) {
		return iter->second;
	}
	return dummyBindings;
}

Vector<std::pair<String, String>> ControlBindings::getConflicts() const
{
	// TODO
	return {};
}

void ControlBindings::apply(InputVirtual& dst, const IControlBindingMapper& mapper, const std::shared_ptr<InputDevice>& mouse, const std::shared_ptr<InputDevice>& keyboard, const Vector<std::shared_ptr<InputDevice>>& gamepads) const
{
	resolve(resolvedBindings.empty());

	dst.clearBindings();

	AxisPendingState pendingState;

	for (const auto& bindingConfig: config.getBindings()) {
		const auto& bs = resolvedBindings.at(bindingConfig.getBindingId());

		for (const auto& binding: bs) {
			if (binding.getBindingType() == ControlBindingType::KeyboardButton) {
				const auto [b0, b1] = binding.getKeyboardButtonIdx();
				applyButtonBinding(dst, mapper, keyboard, b0, b1, bindingConfig, pendingState);
			} else if (binding.getBindingType() == ControlBindingType::MouseButton) {
				auto b0 = binding.getMouseButtonIdx();
				applyButtonBinding(dst, mapper, mouse, b0, std::nullopt, bindingConfig, pendingState);
			} else if (binding.getBindingType() == ControlBindingType::GamepadButton) {
				for (const auto& gamepad: gamepads) {
					const auto [b0, b1] = binding.getGamepadButtonIdx(*gamepad);
					applyButtonBinding(dst, mapper, gamepad, b0, b1, bindingConfig, pendingState);
				}
			} else if (binding.getBindingType() == ControlBindingType::GamepadAxis) {
				for (const auto& gamepad: gamepads) {
					auto [axis, axisDir] = binding.getGamepadAxisIdx(*gamepad);
					applyAxisBinding(dst, mapper, gamepad, axis, axisDir, bindingConfig, pendingState);
				}
			}
		}
	}

	// Pending axes
	for (const auto& axis: pendingState.axisButtons) {
		dst.bindAxisButton(axis.axisId, axis.device, axis.negativeButton.value_or(-1), axis.positiveButton.value_or(-1));
	}
	for (const auto& axis: pendingState.axisAxes) {
		dst.bindAxis(axis.axisId, axis.device, axis.deviceAxis, axis.scale);
	}
}

uint32_t ControlBindings::getVersion() const
{
	return version;
}

void ControlBindings::applyButtonBinding(InputVirtual& dst, const IControlBindingMapper& mapper, const std::shared_ptr<InputDevice>& device, int button, std::optional<int> chordButton, const ControlBindingConfig& bindingConfig, AxisPendingState& pendingState) const
{
	if (bindingConfig.getTargetType() == ControlBindingTargetType::Button) {
		const auto n = mapper.getVirtualButtonId(bindingConfig.getBindingId());
		if (chordButton) {
			dst.bindButtonChord(n, device, *chordButton, button);
		} else {
			dst.bindButton(n, device, button);
		}
	} else if (bindingConfig.getTargetType() == ControlBindingTargetType::Axis) {
		const auto [axisId, axisDir] = ControlBinding::parseAxis(bindingConfig.getBindingId());
		const auto n = mapper.getVirtualAxisId(axisId);
		pendingState.addButton(n, device, button, axisDir);
	}
}

void ControlBindings::applyAxisBinding(InputVirtual& dst, const IControlBindingMapper& mapper, const std::shared_ptr<InputDevice>& device, int axis, JoystickAxisDirection dir, const ControlBindingConfig& bindingConfig, AxisPendingState& pendingState) const
{
	if (bindingConfig.getTargetType() == ControlBindingTargetType::Button) {
		Logger::logError("Binding axis to button is not supported, bindingId = " + bindingConfig.getBindingId());
	} else if (bindingConfig.getTargetType() == ControlBindingTargetType::Axis) {
		const auto [axisId, axisDir] = ControlBinding::parseAxis(bindingConfig.getBindingId());
		const auto n = mapper.getVirtualAxisId(axisId);
		pendingState.addAxis(n, device, axis, axisDir == dir ? 1.0f : -1.0f);
	}
}

void ControlBindings::AxisPendingState::addButton(int axisId, std::shared_ptr<InputDevice> device, int button, JoystickAxisDirection dir)
{
	const auto iter = std_ex::find_if(axisButtons, [&] (AxisButtonPending& axis) {
		return axis.axisId == axisId && axis.device == device;
	});

	AxisButtonPending* dst;
	if (iter != axisButtons.end()) {
		dst = &*iter;
	} else {
		dst = &axisButtons.emplace_back(AxisButtonPending{ axisId, device, {}, {} });
	}

	if (dir == JoystickAxisDirection::Positive) {
		dst->positiveButton = button;
	} else {
		dst->negativeButton = button;
	}
}

void ControlBindings::AxisPendingState::addAxis(int axisId, std::shared_ptr<InputDevice> device, int deviceAxis, float scale)
{
	const auto iter = std_ex::find_if(axisAxes, [&] (AxisAxisPending& axis) {
		return axis.axisId == axisId && axis.device == device && axis.deviceAxis == deviceAxis;
	});

	AxisAxisPending* dst;
	if (iter != axisAxes.end()) {
		dst = &*iter;
	} else {
		dst = &axisAxes.emplace_back(AxisAxisPending{ axisId, device, deviceAxis, scale });
	}

	dst->scale = scale;
}

ControlBinding* ControlBindings::tryGetUserBinding(std::string_view bindingId, size_t slot)
{
	if (slot >= config.getBindingSlots().size()) {
		Logger::logError("Invalid control slot number: " + toString(slot));
		return nullptr;
	}

	if (!config.getBindingIds().contains(bindingId)) {
		Logger::logError("Invalid control binding: " + String(bindingId));
		return nullptr;
	}

	const String key = String(bindingId) + ":" + slot;
	return &userBindings[key];
}

std::optional<ControlBinding> ControlBindings::scanForPress(const InputAPI& inputAPI, const ControlBindingConfig& config, gsl::span<const InputType> acceptedInputTypes)
{
	const auto& inputTypes = config.getInputTypes();
	
	if (inputTypes.contains(InputType::Keyboard) && std_ex::contains(acceptedInputTypes, InputType::Keyboard)) {
		for (size_t i = 0; i < inputAPI.getNumberOfKeyboards(); ++i) {
			if (auto kb = inputAPI.getKeyboard(static_cast<int>(i))) {
				if (auto res = scanForPressKeyboard(*kb)) {
					return res;
				}
			}
		}
	}

	if (inputTypes.contains(InputType::Mouse) && std_ex::contains(acceptedInputTypes, InputType::Mouse)) {
		for (size_t i = 0; i < inputAPI.getNumberOfMice(); ++i) {
			if (auto mouse = inputAPI.getMouse(static_cast<int>(i))) {
				if (auto res = scanForPressMouse(*mouse)) {
					return res;
				}
			}
		}
	}

	if (inputTypes.contains(InputType::Gamepad) && std_ex::contains(acceptedInputTypes, InputType::Gamepad)) {
		for (size_t i = 0; i < inputAPI.getNumberOfJoysticks(); ++i) {
			if (auto gamepad = inputAPI.getJoystick(static_cast<int>(i))) {
				if (auto res = scanForPressGamepad(*gamepad, config.getTargetType() == ControlBindingTargetType::Button, config.getTargetType() == ControlBindingTargetType::Axis)) {
					return res;
				}
			}
		}
	}

	return {};
}

std::optional<ControlBinding> ControlBindings::scanForPress(const InputDevice& device, const ControlBindingConfig& config, gsl::span<const InputType> acceptedInputTypes)
{
	const auto& inputTypes = config.getInputTypes();
	const auto type = device.getInputType();
	if (type == InputType::Gamepad && inputTypes.contains(InputType::Gamepad) && std_ex::contains(acceptedInputTypes, InputType::Gamepad)) {
		return scanForPressGamepad(device, config.getTargetType() == ControlBindingTargetType::Button, config.getTargetType() == ControlBindingTargetType::Axis);
	} else if (type == InputType::Keyboard && inputTypes.contains(InputType::Keyboard) && std_ex::contains(acceptedInputTypes, InputType::Keyboard)) {
		return scanForPressKeyboard(device);
	} else if (type == InputType::Mouse && inputTypes.contains(InputType::Mouse) && std_ex::contains(acceptedInputTypes, InputType::Mouse)) {
		return scanForPressMouse(device);
	}
	return std::nullopt;
}

std::optional<ControlBinding> ControlBindings::scanForPressKeyboard(const InputDevice& keyboard)
{
	for (auto b: keyboard.getButtonsPressed()) {
		return ControlBinding(static_cast<KeyCode>(b));
	}
	return std::nullopt;
}

std::optional<ControlBinding> ControlBindings::scanForPressMouse(const InputDevice& mouse)
{
	for (auto b: mouse.getButtonsPressed()) {
		return ControlBinding(static_cast<MouseButton>(b));
	}
	return std::nullopt;
}

std::optional<ControlBinding> ControlBindings::scanForPressGamepad(const InputDevice& gamepad, bool button, bool axis)
{
	if (button) {
		for (auto b: gamepad.getButtonsPressed()) {
			if (auto pos = gamepad.getPositionForButton(b)) {
				return ControlBinding(*pos);
			}
		}
	}
	
	if (axis) {
		for (auto [axis, dir]: gamepad.getAxesMoved()) {
			if (auto pos = gamepad.getPositionForAxis(axis)) {
				return ControlBinding(*pos, dir);
			}
		}
	}

	return std::nullopt;
}
