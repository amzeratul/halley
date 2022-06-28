#pragma once

#include <memory>
#include <functional>

namespace Halley {
	class SystemMessage;
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
	using CreateMessageByNameFunction = std::function<std::unique_ptr<Message>(const String&)>;
	using CreateSystemMessageFunction = std::function<std::unique_ptr<SystemMessage>(int)>;
	using CreateSystemMessageByNameFunction = std::function<std::unique_ptr<SystemMessage>(const String&)>;

	class CreateEntityFunctions {
	public:
		static CreateComponentFunction& getCreateComponent();
		static CreateSystemFunction& getCreateSystem();
		static CreateMessageFunction& getCreateMessage();
		static CreateMessageByNameFunction& getCreateMessageByName();
		static CreateSystemMessageFunction& getCreateSystemMessage();
		static CreateSystemMessageByNameFunction& getCreateSystemMessageByName();
	};
}
