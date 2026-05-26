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
		ControlBindings(ControlBindingConfigs config);

		void load(const ConfigNode& node);
		ConfigNode toConfigNode() const;

		const ControlBindingConfigs& getConfig() const;

		void resetToDefaults();
		void resetToDefaults(gsl::span<int> slots);

		void bindKeyboard(std::string_view bindingId, size_t slot, KeyCode code);
		void bindGamepadButton(std::string_view bindingId, size_t slot, JoystickButtonPosition button);
		void bindGamepadAxis(std::string_view bindingId, size_t slot, JoystickAxisPosition axis, ControlBindingAxisDirection dir);
		void bindMouseButton(std::string_view bindingId, size_t slot, MouseButton button);
		void unbind(std::string_view bindingId, size_t slot);

		const Vector<ControlBinding>& getBindings(std::string_view bindingId) const;

		void apply(InputVirtual& dst, const IControlBindingMapper& mapper, const std::shared_ptr<InputDevice>& mouse, const std::shared_ptr<InputDevice>& keyboard, const Vector<std::shared_ptr<InputDevice>>& gamepads) const;
		uint32_t getVersion() const;

	private:
		struct AxisButtonPending {
			int axisId;
			std::shared_ptr<InputDevice> device;
			std::optional<int> negativeButton;
			std::optional<int> positiveButton;
		};

		struct AxisAxisPending {
			int axisId;
			std::shared_ptr<InputDevice> device;
			int deviceAxis;
			float scale;
		};

		struct AxisPendingState {
			Vector<AxisButtonPending> axisButtons;
			Vector<AxisAxisPending> axisAxes;

			void addButton(int axisId, std::shared_ptr<InputDevice> device, int button, ControlBindingAxisDirection dir);
			void addAxis(int axisId, std::shared_ptr<InputDevice> device, int deviceAxis, float scale);
		};

		ControlBindingConfigs config;
		HashMap<String, ControlBinding> userBindings;
		Vector<ControlBinding> dummyBindings;

		mutable HashMap<String, Vector<ControlBinding>> resolvedBindings;
		mutable bool modified = false;
		uint32_t version = 0;

		void resolve(bool force) const;
		std::optional<size_t> findSlotIdx(const Vector<ControlBinding>& bs, ControlBindingType type) const;

		ControlBinding* tryGetUserBinding(std::string_view bindingId, size_t slot);

		void applyButtonBinding(InputVirtual& dst, const IControlBindingMapper& mapper, const std::shared_ptr<InputDevice>& device, int button, std::optional<int> chordButton, const ControlBindingConfig& bindingConfig, AxisPendingState& pendingState) const;
		void applyAxisBinding(InputVirtual& dst, const IControlBindingMapper& mapper, const std::shared_ptr<InputDevice>& device, int axis, ControlBindingAxisDirection dir, const ControlBindingConfig& bindingConfig, AxisPendingState& pendingState) const;
	};
}
