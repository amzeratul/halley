#pragma once

#include "registry.h"

namespace Halley {
    using CreateComponentFunction = std::function<CreateComponentFunctionResult(const EntityFactoryContext& context, const String& componentName, EntityRef& entity, const ConfigNode& componentData)>;

	class WorldReflection {
    public:
		WorldReflection() = default;
        explicit WorldReflection(CodegenFunctions& codegenFunctions);

		CreateComponentFunctionResult createComponent(const EntityFactoryContext& context, const String& componentName, EntityRef& entity, const ConfigNode& componentData) const;
	    std::unique_ptr<System> createSystem(const String& name) const;
		std::unique_ptr<Message> createMessage(int id) const;
		std::unique_ptr<Message> createMessage(const String& name) const;
		std::unique_ptr<SystemMessage> createSystemMessage(int id) const;
		std::unique_ptr<SystemMessage> createSystemMessage(const String& name) const;
		ComponentReflector& getComponentReflector(int id) const;

	private:
		Vector<SystemReflector> systemReflectors;
		Vector<std::unique_ptr<ComponentReflector>> componentReflectors;
		Vector<std::unique_ptr<MessageReflector>> messageReflectors;
		Vector<std::unique_ptr<SystemMessageReflector>> systemMessageReflectors;

		HashMap<String, int> systemMap;
		HashMap<String, int> componentMap;
		HashMap<String, int> messageMap;
		HashMap<String, int> systemMessageMap;
    };
}
