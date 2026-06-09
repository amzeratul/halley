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

	class ControlBinding {
	public:
		static std::pair<String, JoystickAxisDirection> parseAxis(std::string_view axisName);

		ControlBinding() = default;
	    ControlBinding(const ConfigNode& node);

		ControlBinding(MouseButton button);
		ControlBinding(KeyCode button, KeyMods mods = KeyMods::None);
		ControlBinding(JoystickButtonPosition button, std::optional<JoystickButtonPosition> chord = {});
		ControlBinding(JoystickAxisPosition axis, JoystickAxisDirection direction);

		ConfigNode toConfigNode() const;

		ControlBindingType getBindingType() const;
		InputType getBindingInputType() const;

		bool bind(MouseButton button);
		bool bind(KeyCode button, KeyMods mods = KeyMods::None);
		bool bind(JoystickButtonPosition button, std::optional<JoystickButtonPosition> chord = {});
		bool bind(JoystickAxisPosition axis, JoystickAxisDirection direction);
		bool unbind();

		std::pair<JoystickAxisPosition, JoystickAxisDirection> getGamepadAxis() const;
		std::pair<JoystickButtonPosition, std::optional<JoystickButtonPosition>> getGamepadButtonPosition() const;
		std::pair<KeyCode, KeyMods> getKeyCode() const;
		MouseButton getMouseButton() const;

		std::pair<int, std::optional<int>> getGamepadButtonIdx(const InputDevice& gamepad) const;
		std::pair<int, JoystickAxisDirection> getGamepadAxisIdx(const InputDevice& gamepad) const;
		std::pair<int, std::optional<int>> getKeyboardButtonIdx() const;
		int getMouseButtonIdx() const;

		[[nodiscard]] ControlBinding convertToGamepadAxis() const;

		bool operator==(const ControlBinding& other) const = default;
		bool operator!=(const ControlBinding& other) const = default;

		uint64_t getHash() const;
		void feedToHash(Hash::Hasher& hasher) const;

	private:
		ControlBindingType bindingType = ControlBindingType::None;

		OptionalLite<MouseButton> mouseButton;
		
		OptionalLite<KeyCode> keyCode;
		OptionalLite<KeyMods> keyMods;
		
		OptionalLite<JoystickButtonPosition> gamepadButton;
		OptionalLite<JoystickButtonPosition> gamepadButtonChord;

		OptionalLite<JoystickAxisPosition> gamepadAxis;
		OptionalLite<JoystickAxisDirection> gamepadAxisDirection;
		
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
		const Vector<String>& getCanOverlap() const;
		const Vector<InputType>& getInputTypes() const;
		const Vector<ControlBinding>& getDefaultBindings() const;
		const Vector<ControlInheritedBinding>& getInheritedBindings() const;
		bool isHidden() const;
		bool isDevOnly() const;
		bool isOptional() const;
		bool isCritical() const;
		bool isToggle() const;
	    
		bool requiresInputType(InputType input) const;
	    bool hasRelevantBindings(const Vector<InputType>& types) const;

    private:
	    String bindingId;
		ControlBindingTargetType bindingTargetType = ControlBindingTargetType::Button;
		String groupId;
	    String exclusivityGroup;
		Vector<String> canOverlap;
		Vector<InputType> inputTypes;
		Vector<ControlBinding> defaultBindings;
		Vector<ControlInheritedBinding> inheritedBindings;
		bool hidden;
		bool devOnly;
		bool optional;
		bool critical;
		bool toggle;
	};

	class ControlBindingConfigs {
	public:
		ControlBindingConfigs() = default;
	    ControlBindingConfigs(const ConfigNode& node);

		const ControlBindingConfig& getBinding(const String& id) const;
		const Vector<ControlBindingConfig>& getBindings() const;
		const Vector<Vector<InputType>>& getBindingSlots() const;
		const HashSet<String>& getBindingIds() const;
		bool areBindingsMutuallyExclusive(const ControlBindingConfig& a, const ControlBindingConfig& b) const;

	private:
		Vector<ControlBindingConfig> bindingConfigs;
		Vector<Vector<InputType>> bindingSlots;
		HashSet<String> bindingIds;
		Vector<Vector<String>> exclusionGroups;
	};
}

namespace std {
	template<>
	struct hash<Halley::ControlBinding>
	{
		size_t operator()(const Halley::ControlBinding& v) const noexcept
		{
			return v.getHash();
		}
	};
}
