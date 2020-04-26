#pragma once

#include <memory>
#include <functional>

namespace Halley {
    class EntityFactory;
	class EntityRef;
	class String;
	class ConfigNode;
	class System;

	class CreateComponentFunctionResult {
	public:
		int componentId = -1;
		bool created = false;
	};
	
    using CreateComponentFunction = std::function<CreateComponentFunctionResult(EntityFactory& factory, const String& componentName, EntityRef& entity, const ConfigNode& componentData)>;
    using CreateSystemFunction = std::function<std::unique_ptr<System>(String)>;

	class CreateEntityFunctions {
	public:
		static CreateComponentFunction& getCreateComponent();
		static CreateSystemFunction& getCreateSystem();
	};
}
