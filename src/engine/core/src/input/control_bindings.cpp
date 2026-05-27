#include "halley/input/control_bindings.h"

#include "../support/StackWalker/StackWalker.h"
#include "halley/input/input_virtual.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

ControlBindings::ControlBindings(ControlBindingConfigs config)
	: config(std::move(config))
{
	resolve(true);
}

void ControlBindings::load(const ConfigNode& node)
{
	userBindings = node["userBindings"].asHashMap<String, ControlBinding>();
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

	const auto& slots = config.getBindingSlots();
	for (size_t i = 0; i < slots.size(); ++i) {
		if (bs[i].getBindingType() == ControlBindingType::None && slots[i].contains(inputType)) {
			return i;
		}
	}
	return std::nullopt;	
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
	// TODO
	return false;
}

void ControlBindings::resolve(bool force) const
{
	if (!modified && !force) {
		return;
	}

	resolvedBindings.clear();
	const auto& nSlots = config.getBindingSlots().size();

	// Bind defaults
	for (const auto& bindingConfig: config.getBindings()) {
		auto& dst = resolvedBindings[bindingConfig.getBindingId()];
		dst.resize(nSlots);

		for (const auto& defBinding: bindingConfig.getDefaultBindings()) {
			if (auto idx = findSlotIdx(dst, defBinding.getBindingType())) {
				dst[*idx] = defBinding;
			} else {
				Logger::logError("Unable to bind default controls for " + bindingConfig.getBindingId());
			}
		}
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

void ControlBindings::bindKeyboard(std::string_view bindingId, size_t slot, KeyCode code)
{
	if (auto* binding = tryGetUserBinding(bindingId, slot)) {
		if (binding->bindKeyboardButton(code)) {
			modified = true;
			++version;
		}
	}
}

void ControlBindings::bindGamepadButton(std::string_view bindingId, size_t slot, JoystickButtonPosition button)
{
	if (auto* binding = tryGetUserBinding(bindingId, slot)) {
		if (binding->bindGamepadButton(button)) {
			modified = true;
			++version;
		}
	}
}

void ControlBindings::bindGamepadAxis(std::string_view bindingId, size_t slot, JoystickAxisPosition axis, JoystickAxisDirection dir)
{
	if (auto* binding = tryGetUserBinding(bindingId, slot)) {
		if (binding->bindGamepadAxis(axis, dir)) {
			modified = true;
			++version;
		}
	}
}

void ControlBindings::bindMouseButton(std::string_view bindingId, size_t slot, MouseButton button)
{
	if (auto* binding = tryGetUserBinding(bindingId, slot)) {
		if (binding->bindMouseButton(button)) {
			modified = true;
			++version;
		}
	}
}

void ControlBindings::unbind(std::string_view bindingId, size_t slot)
{
	if (auto* binding = tryGetUserBinding(bindingId, slot)) {
		if (binding->unbind()) {
			modified = true;
			++version;
		}
	}
}

const Vector<ControlBinding>& ControlBindings::getBindings(std::string_view bindingId) const
{
	resolve(resolvedBindings.empty());
	if (const auto iter = resolvedBindings.find(bindingId); iter != resolvedBindings.end()) {
		return iter->second;
	}
	return dummyBindings;
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
