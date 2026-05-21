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

	enum class ControlBindingTargetType : uint8_t {
		Button,
		Axis
	};
	
	template <>
	struct EnumNames<ControlBindingTargetType> {
		constexpr auto operator()() const {
			return std::to_array({
				"button",
				"axis"
			});
		}
	};

	enum class ControlBindingAxisDirection : uint8_t {
		Positive,
		Negative
	};
	
	template <>
	struct EnumNames<ControlBindingAxisDirection> {
		constexpr auto operator()() const {
			return std::to_array({
				"positive",
				"negative"
			});
		}
	};

	class ControlBinding {
	public:
		static std::pair<String, ControlBindingAxisDirection> parseAxis(std::string_view axisName);

		ControlBinding() = default;
	    ControlBinding(const ConfigNode& node);

		ConfigNode toConfigNode() const;

		ControlBindingType getBindingType() const;

		void bindMouseButton(MouseButton button);
		void bindKeyboardButton(KeyCode button, std::optional<KeyCode> chord = {});
		void bindGamepadButton(JoystickButtonPosition button, std::optional<JoystickButtonPosition> chord = {});
		void bindGamepadAxis(JoystickAxisPosition axis, ControlBindingAxisDirection direction);
		void unbind();

		std::pair<JoystickAxisPosition, ControlBindingAxisDirection> getGamepadAxis() const;
		std::pair<JoystickButtonPosition, std::optional<JoystickButtonPosition>> getGamepadButtonPosition() const;
		std::pair<KeyCode, std::optional<KeyCode>> getKeyCode() const;
		MouseButton getMouseButton() const;

		std::pair<int, std::optional<int>> getGamepadButtonIdx(const InputDevice& gamepad) const;
		std::pair<int, ControlBindingAxisDirection> getGamepadAxisIdx(const InputDevice& gamepad) const;
		std::pair<int, std::optional<int>> getKeyboardButtonIdx() const;
		int getMouseButtonIdx() const;

		[[nodiscard]] ControlBinding convertToGamepadAxis() const;

	private:
		ControlBindingType bindingType = ControlBindingType::None;

		std::optional<MouseButton> mouseButton;
		
		std::optional<KeyCode> keyCode;
		std::optional<KeyCode> keyCodeChord;
		
		std::optional<JoystickButtonPosition> gamepadButton;
		std::optional<JoystickButtonPosition> gamepadButtonChord;

		std::optional<JoystickAxisPosition> gamepadAxis;
		std::optional<ControlBindingAxisDirection> gamepadAxisDirection;
		
		void loadValue(ControlBindingType type, const ConfigNode& value);
	};

	class ControlInheritedBinding {
	public:
		ControlInheritedBinding() = default;
	    ControlInheritedBinding(const ConfigNode& node);
		
		ControlBindingType getBindingType() const;
		const String& getSourceId() const;

	private:
		ControlBindingType bindingType = ControlBindingType::None;
		String sourceId;
	};

	class ControlBindingConfig {
	public:
	    ControlBindingConfig() = default;
	    ControlBindingConfig(const ConfigNode& node);

	    const String& getBindingId() const;
		ControlBindingTargetType getTargetType() const;
	    const String& getGroupId() const;
	    const String& getExclusivityGroup() const;
		const Vector<InputType>& getInputTypes() const;
		const Vector<ControlBinding>& getDefaultBindings() const;
		const Vector<ControlInheritedBinding>& getInheritedBindings() const;
		bool isHidden() const;
		bool isDevOnly() const;

	private:
	    String bindingId;
		ControlBindingTargetType bindingTargetType = ControlBindingTargetType::Button;
		String groupId;
	    String exclusivityGroup;
		Vector<InputType> inputTypes;
		Vector<ControlBinding> defaultBindings;
		Vector<ControlInheritedBinding> inheritedBindings;
		bool hidden;
		bool devOnly;
	};

	class ControlBindingConfigs {
	public:
		ControlBindingConfigs() = default;
	    ControlBindingConfigs(const ConfigNode& node);

		const Vector<ControlBindingConfig>& getBindings() const;
		const Vector<Vector<InputType>>& getBindingSlots() const;

	private:
		Vector<ControlBindingConfig> bindingConfigs;
		Vector<Vector<InputType>> bindingSlots;
	};
}
