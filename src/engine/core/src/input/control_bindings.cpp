#include "halley/input/control_bindings.h"

#include "../support/StackWalker/StackWalker.h"
#include "halley/input/input_virtual.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

ControlBindings::ControlBindings(ControlBindingConfigs config)
	: config(std::move(config))
{
	loadDefaults();
}

void ControlBindings::load(const ConfigNode& node)
{
	// TODO
	//bindings = node["bindings"].asHashMap<String, Vector<ControlBinding>>();
	resolve();
}

ConfigNode ControlBindings::toConfigNode() const
{
	// TODO
	ConfigNode result;
	//result["bindings"] = bindings;
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

void ControlBindings::loadDefaults()
{
	bindings.clear();
	const auto& nSlots = config.getBindingSlots().size();

	// Bind defaults
	for (const auto& bindingConfig: config.getBindings()) {
		auto& dst = bindings[bindingConfig.getBindingId()];
		dst.resize(nSlots);

		for (const auto& defBinding: bindingConfig.getDefaultBindings()) {
			if (auto idx = findSlotIdx(dst, defBinding.getBindingType())) {
				dst[*idx] = defBinding;
			} else {
				Logger::logError("Unable to bind default controls for " + bindingConfig.getBindingId());
			}
		}
	}

	modified = true;
	++version;
	resolve();
}

void ControlBindings::resolve() const
{
	if (!modified) {
		return;
	}

	resolvedBindings = bindings;

	// Bind inherited
	for (const auto& bindingConfig: config.getBindings()) {
		auto& dst = resolvedBindings[bindingConfig.getBindingId()];

		for (const auto& inhBinding: bindingConfig.getInheritedBindings()) {
			if (auto idx = findSlotIdx(dst, inhBinding.getBindingType())) {
				const auto& src = resolvedBindings.at(inhBinding.getSourceId());
				for (const auto& srcBinding: src) {
					if (srcBinding.getBindingType() == inhBinding.getBindingType()) {
						dst[*idx] = srcBinding;
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
	if (auto* binding = tryGetBinding(bindingId, slot)) {
		binding->bindKeyboardButton(code);
		modified = true;
		++version;
	}
}

void ControlBindings::bindGamepadButton(std::string_view bindingId, size_t slot, JoystickButtonPosition button)
{
	if (auto* binding = tryGetBinding(bindingId, slot)) {
		binding->bindGamepadButton(button);
		modified = true;
		++version;
	}
}

void ControlBindings::bindGamepadAxis(std::string_view bindingId, size_t slot, JoystickAxisPosition axis, ControlBindingAxisDirection dir)
{
	if (auto* binding = tryGetBinding(bindingId, slot)) {
		binding->bindGamepadAxis(axis, dir);
		modified = true;
		++version;
	}
}

void ControlBindings::bindMouseButton(std::string_view bindingId, size_t slot, MouseButton button)
{
	if (auto* binding = tryGetBinding(bindingId, slot)) {
		binding->bindMouseButton(button);
		modified = true;
		++version;
	}
}

void ControlBindings::unbind(std::string_view bindingId, size_t slot)
{
	if (auto* binding = tryGetBinding(bindingId, slot)) {
		binding->unbind();
		modified = true;
		++version;
	}
}

const Vector<ControlBinding>& ControlBindings::getBindings(std::string_view bindingId) const
{
	resolve();
	if (const auto iter = resolvedBindings.find(bindingId); iter != resolvedBindings.end()) {
		return iter->second;
	}
	return dummyBindings;
}

void ControlBindings::apply(InputVirtual& dst, const IControlBindingMapper& mapper, const std::shared_ptr<InputDevice>& mouse, const std::shared_ptr<InputDevice>& keyboard, const Vector<std::shared_ptr<InputDevice>>& gamepads) const
{
	resolve();

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

void ControlBindings::applyAxisBinding(InputVirtual& dst, const IControlBindingMapper& mapper, const std::shared_ptr<InputDevice>& device, int axis, ControlBindingAxisDirection dir, const ControlBindingConfig& bindingConfig, AxisPendingState& pendingState) const
{
	if (bindingConfig.getTargetType() == ControlBindingTargetType::Button) {
		Logger::logError("Binding axis to button is not supported, bindingId = " + bindingConfig.getBindingId());
	} else if (bindingConfig.getTargetType() == ControlBindingTargetType::Axis) {
		const auto [axisId, axisDir] = ControlBinding::parseAxis(bindingConfig.getBindingId());
		const auto n = mapper.getVirtualAxisId(axisId);
		pendingState.addAxis(n, device, axis, axisDir == dir ? 1.0f : -1.0f);
	}
}

void ControlBindings::AxisPendingState::addButton(int axisId, std::shared_ptr<InputDevice> device, int button, ControlBindingAxisDirection dir)
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

	if (dir == ControlBindingAxisDirection::Positive) {
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

ControlBinding* ControlBindings::tryGetBinding(std::string_view bindingId, size_t slot)
{
	if (const auto iter = bindings.find(bindingId); iter != bindings.end()) {
		auto& bs = iter->second;

		if (slot < bs.size()) {
			return &bs[slot];
		}
	}
	return nullptr;
}
