#pragma once
#include "halley/data_structures/config_node.h"

namespace Halley {
	class ConstEntityRef;
	class EntityFactoryContext;
	class SystemMessage;
	class System;
	class Message;
	class EntityRef;
	class Component;
	class EntitySerializationContext;

	class CreateComponentFunctionResult {
	public:
		int componentId = -1;
		bool created = false;
	};

	class ComponentReflector {
    public:
    	virtual ~ComponentReflector() = default;

    	virtual const char* getName() const = 0;
		virtual int getIndex() const = 0;

		virtual ConfigNode serialize(const EntitySerializationContext& context, const Component& component) const = 0;
		virtual CreateComponentFunctionResult createComponent(const EntityFactoryContext& context, EntityRef& e, const ConfigNode& node) const = 0;

		virtual ConfigNode serializeField(const EntitySerializationContext& context, const Component& component, std::string_view fieldName) const = 0;
		virtual ConfigNode serializeField(const EntitySerializationContext& context, EntityRef entity, std::string_view fieldName) const = 0;
		virtual ConfigNode serializeField(const EntitySerializationContext& context, ConstEntityRef entity, std::string_view fieldName) const = 0;
    	virtual void deserializeField(const EntitySerializationContext& context, Component& component, std::string_view fieldName, const ConfigNode& data) const = 0;
    	virtual void deserializeField(const EntitySerializationContext& context, EntityRef entity, std::string_view fieldName, const ConfigNode& data) const = 0;

		virtual Component* tryGetComponent(EntityRef entity) const = 0;
		virtual const Component* tryGetComponent(ConstEntityRef entity) const = 0;

		virtual void rebindComponent(Component& component, EntityRef entity) const = 0;
    };

	class MessageReflector {
    public:
    	virtual ~MessageReflector() = default;

    	virtual const char* getName() const = 0;
		virtual int getIndex() const = 0;
		virtual std::unique_ptr<Message> createMessage() const = 0;
    };

	class SystemMessageReflector {
    public:
    	virtual ~SystemMessageReflector() = default;

    	virtual const char* getName() const = 0;
		virtual int getIndex() const = 0;
		virtual std::unique_ptr<SystemMessage> createSystemMessage() const = 0;
    };

	class SystemReflector {
	public:
		using MakeSystemPtr = System*(*)();

		SystemReflector() = default;
		SystemReflector(String name, MakeSystemPtr makeSystem)
			: name(std::move(name))
			, makeSystem(makeSystem)
		{}

		const String& getName() const { return name; }
		System* createSystem() const { return makeSystem(); }
		const SystemReflector* operator->() const { return this; }

	private:
		String name;
		MakeSystemPtr makeSystem = nullptr;
	};
}
