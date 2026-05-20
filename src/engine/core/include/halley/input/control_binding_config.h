#pragma once
#include "input_device.h"

namespace Halley {
	class ConfigNode;

	enum class ControlBindingType : uint8_t {
		None,
		KeyboardButton,
		MouseButton,
		GamepadButton,
		GamepadAxis
	};

	template <>
	struct EnumNames<ControlBindingType> {
		constexpr auto operator()() const {
			return std::to_array({
				"none",
				"keyboardButton",
				"mouseButton",
				"gamepadButton",
				"gamepadAxis"
			});
		}
	};

	class ControlBinding {
	public:
		ControlBinding() = default;
	    ControlBinding(const ConfigNode& node);

		ConfigNode toConfigNode() const;

		ControlBindingType getBindingType() const;

		void bindMouseButton(MouseButton button);
		void bindKeyboardButton(KeyCode button);
		void bindGamepadButton(JoystickButtonPosition button);
		void bindGamepadAxis(JoystickAxisPosition axis);
		void unbind();

		JoystickAxisPosition getJoystickAxis() const;
		JoystickButtonPosition getJoystickButtonPosition() const;
		MouseButton getMouseButton() const;
		KeyCode getKeyCode() const;

	private:
		ControlBindingType bindingType = ControlBindingType::None;

		MouseButton mouseButton = MouseButton::Left;
		KeyCode keyCode = KeyCode::Unknown;
		JoystickButtonPosition gamepadButton = JoystickButtonPosition::PlatformAcceptButton;
		JoystickAxisPosition gamepadAxis = JoystickAxisPosition::LeftStickX;
	};

	class ControlBindingConfig {
	public:
	    ControlBindingConfig() = default;
	    ControlBindingConfig(const ConfigNode& node);

	    const String& getBindingId() const;
	    const String& getExclusivityGroup() const;
		const Vector<InputType>& getInputTypes() const;
		const Vector<ControlBinding>& getDefaultBindings() const;

	private:
	    String bindingId;
	    String exclusivityGroup;
		Vector<InputType> inputTypes;
		Vector<ControlBinding> defaultBindings;
	};

	class ControlBindingConfigs {
	public:
		ControlBindingConfigs() = default;
	    ControlBindingConfigs(const ConfigNode& node);

		const Vector<ControlBindingConfig>& getBindings() const;

	private:
		Vector<ControlBindingConfig> bindingConfigs;
	};
}
