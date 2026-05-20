#pragma once
#include "halley/data_structures/config_node.h"
#include "control_binding_config.h"

namespace Halley {
	class InputVirtual;

	class ControlBindings {
	public:
		ControlBindings(const ControlBindingConfigs& config);

		void load(const ConfigNode& node);
		ConfigNode toConfigNode() const;

		const ControlBindingConfigs& getConfig() const;

		void loadDefaults();
		void bindKeyboard(std::string_view bindingId, size_t slot, KeyCode code);
		void bindGamepadButton(std::string_view bindingId, size_t slot, JoystickButtonPosition button);
		void bindGamepadAxis(std::string_view bindingId, size_t slot, JoystickAxisPosition axis);
		void bindMouseButton(std::string_view bindingId, size_t slot, MouseButton button);
		void unbind(std::string_view bindingId, size_t slot);

		const Vector<ControlBinding>& getBindings(std::string_view bindingId) const;

		void apply(InputVirtual& dst);

	private:
		const ControlBindingConfigs& config;
		HashMap<String, Vector<ControlBinding>> bindings;
		Vector<ControlBinding> dummyBindings;

		ControlBinding* tryGetBinding(std::string_view bindingId, size_t slot);
	};
}
