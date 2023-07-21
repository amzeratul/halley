#pragma once
#include "ecs_reflection.h"
#include "halley/data_structures/config_node.h"
#include "halley/entity/entity_factory.h"

namespace Halley {
	template <typename T>
	class ComponentReflectorImpl : public ComponentReflector {
	public:
		const char* getName() const override
		{
			return T::componentName;
		}

		int getIndex() const override
		{
			return T::componentIndex;
		}
		
		ConfigNode serialize(const EntitySerializationContext& context, const Component& component) const override
		{
			return static_cast<const T&>(component).serialize(context);
		}

		CreateComponentFunctionResult createComponent(const EntityFactoryContext& context, EntityRef& e, const ConfigNode& node) const override
		{
			return context.createComponent<T>(e, node);
		}
	};

	template <typename T>
	class MessageReflectorImpl : public MessageReflector {
	public:
		const char* getName() const override
		{
			return T::messageName;
		}

		int getIndex() const override
		{
			return T::messageIndex;
		}
		
		std::unique_ptr<Message> createMessage() const override
		{
			return std::make_unique<T>();
		}
	};

	template <typename T>
	class SystemMessageReflectorImpl : public SystemMessageReflector {
	public:
		const char* getName() const override
		{
			return T::messageName;
		}

		int getIndex() const override
		{
			return T::messageIndex;
		}
		
		std::unique_ptr<SystemMessage> createSystemMessage() const override
		{
			return std::make_unique<T>();
		}
	};
}
