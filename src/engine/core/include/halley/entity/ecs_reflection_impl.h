#pragma once
#include "ecs_reflection.h"
#include "halley/data_structures/config_node.h"
#include "halley/entity/entity_factory.h"

namespace Halley {
	template <typename T>
	class ComponentReflectorImpl final : public ComponentReflector {
	public:
		const char* getName() const override
		{
			return T::componentName;
		}

		int getIndex() const override
		{
			return T::componentIndex;
		}

		uint32_t getQuickIndex() const override
		{
			return T::quickIndex;
		}
		
		ConfigNode serialize(const EntitySerializationContext& context, const Component& component) const override
		{
			ConfigNode result = static_cast<const T&>(component).serialize(context);
			result.ensureType(ConfigNodeType::DeltaMap);
			return result;
		}

		void hash(Hash::Hasher& hasher, const EntitySerializationContext& context, const Component& component) const override
		{
			static_cast<const T&>(component).hash(context, hasher);
		}

		CreateComponentFunctionResult createComponent(const EntityFactoryContext& context, EntityRef& e, const ConfigNode& node) const override
		{
			return context.createComponent<T>(e, node);
		}

		ConfigNode serializeField(const EntitySerializationContext& context, const Component& component, std::string_view fieldName) const override
		{
			return static_cast<const T&>(component).serializeField(context, fieldName);
		}

		ConfigNode serializeField(const EntitySerializationContext& context, EntityRef entity, std::string_view fieldName, bool evenIfDisabled, bool logError) const override
		{
			if (const auto* component = tryGetComponent(entity, evenIfDisabled)) {
				return serializeField(context, *component, fieldName);
			} else {
				if (logError) {
					Logger::logError("Component " + String(T::componentName) + " not found in entity " + entity.getName());
				}
				return {};
			}
		}
		
		ConfigNode serializeField(const EntitySerializationContext& context, ConstEntityRef entity, std::string_view fieldName, bool evenIfDisabled, bool logError) const override
		{
			if (const auto* component = tryGetComponent(entity, evenIfDisabled)) {
				return serializeField(context, *component, fieldName);
			} else {
				if (logError) {
					Logger::logError("Component " + String(T::componentName) + " not found in entity " + entity.getName());
				}
				return {};
			}
		}

		void deserializeField(const EntitySerializationContext& context, Component& component, std::string_view fieldName, const ConfigNode& data) const override
		{
			static_cast<T&>(component).deserializeField(context, fieldName, data);
		}

		void deserializeField(const EntitySerializationContext& context, EntityRef entity, std::string_view fieldName, bool evenIfDisabled, const ConfigNode& data) const override
		{
			if (auto* component = tryGetComponent(entity, evenIfDisabled)) {
				deserializeField(context, *component, fieldName, data);
			} else {
				Logger::logError("Component " + String(T::componentName) + " not found in entity " + entity.getName());
			}
		}

		void sanitize(ConfigNode& data, int mask) const override
		{
			T::sanitize(data, mask);
		}

        void serializeNetwork(const ByteSerializationContext& context, Serializer& serializer, const Component& component) const override
        {
            static_cast<const T&>(component).serializeNetwork(context, serializer);
        }

        void deserializeNetwork(const ByteSerializationContext& context, Deserializer& deserializer, Component& component) const override
        {
            static_cast<T&>(component).deserializeNetwork(context, deserializer);
        }

        Component* tryGetComponent(EntityRef entity, bool evenIfDisabled) const override
		{
			return entity.tryGetComponent<T>(evenIfDisabled);
		}

		const Component* tryGetComponent(ConstEntityRef entity, bool evenIfDisabled) const override
		{
			return entity.tryGetComponent<T>(evenIfDisabled);
		}

		Component* createComponent(EntityRef entity) const override
		{
			T component;
			entity.addComponent<T>(std::move(component));
			return entity.tryGetComponent<T>(false);
		}

		void rebindComponent(Component& component, EntityRef entity) const override
		{
			if constexpr (HasOnAddedToEntityMember<T>::value) {
				static_cast<T&>(component).onAddedToEntity(entity);
			}
		}

		bool isAlwaysEnabled() const override
		{
			return T::alwaysEnabled;
		}
	};

	template <typename T>
	class MessageReflectorImpl final : public MessageReflector {
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
	class SystemMessageReflectorImpl final : public SystemMessageReflector {
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
