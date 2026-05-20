#include "halley/input/control_bindings.h"

using namespace Halley;

ControlBindings::ControlBindings(const ControlBindingConfigs& config)
	: config(config)
{
}

void ControlBindings::load(const ConfigNode& node)
{
	bindings = node["bindings"].asHashMap<String, Vector<ControlBinding>>();
}

ConfigNode ControlBindings::toConfigNode() const
{
	ConfigNode result;
	result["bindings"] = bindings;
	return result;
}

const ControlBindingConfigs& ControlBindings::getConfig() const
{
	return config;
}

void ControlBindings::loadDefaults()
{
	// TODO
}

void ControlBindings::bindKeyboard(std::string_view bindingId, size_t slot, KeyCode code)
{
	if (auto* binding = tryGetBinding(bindingId, slot)) {
		binding->bindKeyboardButton(code);
	}
}

void ControlBindings::bindGamepadButton(std::string_view bindingId, size_t slot, JoystickButtonPosition button)
{
	if (auto* binding = tryGetBinding(bindingId, slot)) {
		binding->bindGamepadButton(button);
	}
}

void ControlBindings::bindGamepadAxis(std::string_view bindingId, size_t slot, JoystickAxisPosition axis)
{
	if (auto* binding = tryGetBinding(bindingId, slot)) {
		binding->bindGamepadAxis(axis);
	}
}

void ControlBindings::bindMouseButton(std::string_view bindingId, size_t slot, MouseButton button)
{
	if (auto* binding = tryGetBinding(bindingId, slot)) {
		binding->bindMouseButton(button);
	}
}

void ControlBindings::unbind(std::string_view bindingId, size_t slot)
{
	if (auto* binding = tryGetBinding(bindingId, slot)) {
		binding->unbind();
	}
}

const Vector<ControlBinding>& ControlBindings::getBindings(std::string_view bindingId) const
{
	if (const auto iter = bindings.find(bindingId); iter != bindings.end()) {
		return iter->second;
	}
	return dummyBindings;
}

void ControlBindings::apply(InputVirtual& dst)
{
	// TODO
}

ControlBinding* ControlBindings::tryGetBinding(std::string_view bindingId, size_t slot)
{
	if (const auto iter = bindings.find(bindingId); iter != bindings.end()) {
		auto& bs = iter->second;

		// TODO: resize if in range
		if (slot < bs.size()) {
			return &bs[slot];
		}
	}
	return nullptr;
}
