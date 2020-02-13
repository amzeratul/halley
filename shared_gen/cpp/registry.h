#pragma once

namespace Halley {
	std::unique_ptr<System> createSystem(String name);
	void createComponent(EntityFactory& factory, const String& name, EntityRef& entity, const ConfigNode& componentData);
}
