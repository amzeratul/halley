#pragma once

#include <memory>
#include <functional>

namespace Halley {
	class Message;
	class EntityFactoryContext;
	class EntityRef;
	class String;
	class ConfigNode;
	class System;

	class CreateComponentFunctionResult {
	public:
		int componentId = -1;
		bool created = false;
	};
	
    using CreateComponentFunction = std::function<CreateComponentFunctionResult(const EntityFactoryContext& context, const String& componentName, EntityRef& entity, const ConfigNode& componentData)>;
    using CreateSystemFunction = std::function<std::unique_ptr<System>(String)>;
	using CreateMessageFunction = std::function<std::unique_ptr<Message>(int)>;

	class CreateEntityFunctions {
	public:
		static CreateComponentFunction& getCreateComponent();
		static CreateSystemFunction& getCreateSystem();
		static CreateMessageFunction& getCreateMessage();
	};
}
