#pragma once

#include "registry.h"

namespace Halley {
    using CreateComponentFunction = std::function<CreateComponentFunctionResult(const EntityFactoryContext& context, std::string_view componentName, EntityRef& entity, const ConfigNode& componentData)>;

	class WorldReflection {
    public:
		WorldReflection() = default;
        explicit WorldReflection(CodegenFunctions& codegenFunctions);

		CreateComponentFunctionResult createComponent(const EntityFactoryContext& context, std::string_view componentName, EntityRef& entity, const ConfigNode& componentData) const;
	    std::unique_ptr<System> createSystem(std::string_view name) const;
		std::unique_ptr<Message> createMessage(int id) const;
		std::unique_ptr<Message> createMessage(std::string_view name) const;
		std::unique_ptr<SystemMessage> createSystemMessage(int id) const;
		std::unique_ptr<SystemMessage> createSystemMessage(std::string_view name) const;

		ComponentReflector& getComponentReflector(int id) const;
		const ComponentReflector* tryGetComponentReflector(int id) const;
		ComponentReflector& getComponentReflector(std::string_view name) const;
		MessageReflector& getMessageReflector(int id) const;
		SystemMessageReflector& getSystemMessageReflector(int id) const;

		Vector<int> getAlwaysEnabledComponents() const;

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
