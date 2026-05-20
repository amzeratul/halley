#pragma once
#include "halley/data_structures/config_node.h"
#include "control_binding_config.h"

namespace Halley {
	class InputVirtual;

	class IControlBindingMapper {
	public:
		virtual ~IControlBindingMapper() = default;
		virtual int getVirtualButtonId(std::string_view id) const = 0;
		virtual int getVirtualAxisId(std::string_view id) const = 0;
	};

	class ControlBindings {
	public:
		ControlBindings(const ControlBindingConfigs& config);

		void load(const ConfigNode& node);
		ConfigNode toConfigNode() const;

		const ControlBindingConfigs& getConfig() const;

		void loadDefaults();
		void bindKeyboard(std::string_view bindingId, size_t slot, KeyCode code);
		void bindGamepadButton(std::string_view bindingId, size_t slot, JoystickButtonPosition button);
		void bindGamepadAxis(std::string_view bindingId, size_t slot, JoystickAxisPosition axis, ControlBindingAxisDirection dir);
		void bindMouseButton(std::string_view bindingId, size_t slot, MouseButton button);
		void unbind(std::string_view bindingId, size_t slot);

		const Vector<ControlBinding>& getBindings(std::string_view bindingId) const;

		void apply(InputVirtual& dst, const IControlBindingMapper& mapper, const std::shared_ptr<InputDevice>& mouse, const std::shared_ptr<InputDevice>& keyboard, const Vector<std::shared_ptr<InputDevice>>& gamepads) const;

	private:
		const ControlBindingConfigs& config;
		HashMap<String, Vector<ControlBinding>> bindings;
		Vector<ControlBinding> dummyBindings;

		struct AxisPending {
			int axisId;
			std::shared_ptr<InputDevice> device;
			std::optional<int> negativeButton;
			std::optional<int> positiveButton;
		};

		struct AxisPendingState {
			Vector<AxisPending> axes;

			void addButton(int axisId, std::shared_ptr<InputDevice> device, int button, ControlBindingAxisDirection dir);
		};

		ControlBinding* tryGetBinding(std::string_view bindingId, size_t slot);

		void applyButtonBinding(InputVirtual& dst, const IControlBindingMapper& mapper, const std::shared_ptr<InputDevice>& device, int button, const ControlBindingConfig& bindingConfig, AxisPendingState& pendingState) const;
		void applyAxisBinding(InputVirtual& dst, const IControlBindingMapper& mapper, const std::shared_ptr<InputDevice>& device, int axis, ControlBindingAxisDirection dir, const ControlBindingConfig& bindingConfig) const;
	};
}
