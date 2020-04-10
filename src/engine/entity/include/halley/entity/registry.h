#pragma once

#include <memory>

namespace Halley {
    class System;
    class String;
    class ConfigNode;
    class CreateComponentFunctionResult;

	std::unique_ptr<System> createSystem(String name);
	CreateComponentFunctionResult createComponent(EntityFactory& factory, const String& name, EntityRef& entity, const ConfigNode& componentData);
}
