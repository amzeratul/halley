#pragma once

#include <array>
#include <memory>
#include "component.h"
#include "message.h"
#include "family_mask.h"
#include "entity_id.h"
#include "type_deleter.h"
#include <halley/data_structures/vector.h>
#include "halley/utils/type_traits.h"
#include "halley/maths/uuid.h"

namespace Halley {
	class World;
	class System;
	class EntityRef;

	// True if T::onAddedToEntity(EntityRef&) exists
	template <class, class = void_t<>> struct HasOnAddedToEntityMember : std::false_type {};
	template <class T> struct HasOnAddedToEntityMember<T, decltype(std::declval<T&>().onAddedToEntity(std::declval<EntityRef&>()))> : std::true_type { };
	
	class MessageEntry
	{
	public:
		std::unique_ptr<Message> msg;
		int type = -1;
		int age = -1;

		MessageEntry() {}
		MessageEntry(std::unique_ptr<Message> msg, int type, int age) : msg(std::move(msg)), type(type), age(age) {}
	};

	class EntityRef;
	class ConstEntityRef;

	class Entity
	{
		friend class World;
		friend class System;
		friend class EntityRef;
		friend class ConstEntityRef;

	public:
		~Entity();

		template <typename T>
		T* tryGetComponent()
		{
			constexpr int id = FamilyMask::RetrieveComponentIndex<T>::componentIndex;
			for (int i = 0; i < liveComponents; i++) {
				if (components[i].first == id) {
					return static_cast<T*>(components[i].second);
				}
			}
			return nullptr;
		}

		template <typename T>
		const T* tryGetComponent() const
		{
			constexpr int id = FamilyMask::RetrieveComponentIndex<T>::componentIndex;
			for (int i = 0; i < liveComponents; i++) {
				if (components[i].first == id) {
					return static_cast<const T*>(components[i].second);
				}
			}
			return nullptr;
		}

		template <typename T>
		T& getComponent()
		{
			auto value = tryGetComponent<T>();
			if (value) {
				return *value;
			} else {
				throw Exception("Component " + String(typeid(T).name()) + " does not exist in entity.", HalleyExceptions::Entity);
			}
		}

		template <typename T>
		const T& getComponent() const
		{
			auto value = tryGetComponent<T>();
			if (value) {
				return *value;
			} else {
				throw Exception("Component " + String(typeid(T).name()) + " does not exist in entity.", HalleyExceptions::Entity);
			}
		}

		template <typename T>
		bool hasComponent() const
		{
			if (dirty) {
				return tryGetComponent<T>() != nullptr;
			} else {
				return FamilyMask::hasBit(mask, FamilyMask::RetrieveComponentIndex<T>::componentIndex);
			}
		}

		bool needsRefresh() const
		{
			return dirty;
		}

		bool isAlive() const
		{
			return alive;
		}
		
		const UUID& getUUID() const
		{
			return uuid;
		}

		FamilyMaskType getMask() const;
		EntityId getEntityId() const;

		void refresh(MaskStorage& storage, ComponentDeleterTable& table);
		void destroy();

	private:
		// Start with these for better cache coherence
		Vector<std::pair<int, Component*>> components;
		int liveComponents = 0;
		bool dirty = false;
		bool alive = true;

		int8_t hierarchyRevision = 0;
		Entity* parent = nullptr;
		Vector<Entity*> children;
		
		Vector<MessageEntry> inbox;
		FamilyMaskType mask;
		EntityId uid;
		String name;
		UUID uuid;

		Entity();
		void destroyComponents(ComponentDeleterTable& storage);

		template <typename T>
		Entity& addComponent(World& world, T* component)
		{
			addComponent(component, T::componentIndex);
			TypeDeleter<T>::initialize(getComponentDeleterTable(world));

			markDirty(world);
			return *this;
		}

		template <typename T>
		Entity& removeComponent(World& world)
		{
			constexpr int id = T::componentIndex;
			for (int i = 0; i < liveComponents; ++i) {
				if (components[i].first == id) {
					removeComponentAt(i);
					markDirty(world);
					return *this;
				}
			}

			return *this;
		}

		void addComponent(Component* component, int id);
		void removeComponentAt(int index);
		void removeAllComponents(World& world);
		void deleteComponent(Component* component, int id, ComponentDeleterTable& table);
		void keepOnlyComponentsWithIds(const std::vector<int>& ids, World& world);

		void onReady();

		void markDirty(World& world);
		ComponentDeleterTable& getComponentDeleterTable(World& world);

		Entity* getParent() const { return parent; }
		void setParent(Entity* parent, bool propagate = true);
		const std::vector<Entity*>& getChildren() const { return children; }
		void addChild(Entity& child);
		void detachChildren();
		void markHierarchyDirty();

		void doDestroy(bool updateParenting);
	};

	class EntityRef
	{
	public:
		EntityRef() = default;
		EntityRef(const EntityRef& other) = default;
		EntityRef(EntityRef&& other) = default;

		EntityRef& operator=(const EntityRef& other) = default;
		EntityRef& operator=(EntityRef&& other) = default;
		
		EntityRef(Entity& e, World& w)
			: entity(&e)
			, world(&w)
		{}

		~EntityRef() = default;

		template <typename T>
		EntityRef& addComponent(T&& component)
		{
			static_assert(!std::is_pointer<T>::value, "Cannot pass pointer to component");
			static_assert(!std::is_same<T, Component>::value, "Cannot add base class Component to entity, make sure type isn't being erased");
			static_assert(std::is_base_of<Component, T>::value, "Components must extend the Component class");
			static_assert(!std::is_polymorphic<T>::value, "Components cannot be polymorphic (i.e. they can't have virtual methods)");
			static_assert(std::is_default_constructible<T>::value, "Components must have a default constructor");

			auto c = new T(std::move(component));
			entity->addComponent(*world, c);

			if constexpr (HasOnAddedToEntityMember<T>::value) {
				c->onAddedToEntity(*this);
			}

			return *this;
		}

		template <typename T>
		EntityRef& removeComponent()
		{
			entity->removeComponent<T>(*world);
			return *this;
		}

		EntityRef& removeAllComponents()
		{
			entity->removeAllComponents(*world);
			return *this;
		}

		template <typename T>
		T& getComponent()
		{
			return entity->getComponent<T>();
		}

		template <typename T>
		const T& getComponent() const
		{
			return entity->getComponent<T>();
		}

		template <typename T>
		T* tryGetComponent()
		{
			return entity->tryGetComponent<T>();
		}

		template <typename T>
		const T* tryGetComponent() const
		{
			return entity->tryGetComponent<T>();
		}

		EntityId getEntityId() const
		{
			return entity->getEntityId();
		}

		template <typename T>
		bool hasComponent() const
		{
			return entity->hasComponent<T>();
		}

		const String& getName() const
		{
			return entity->name;
		}

		void setName(String name)
		{
			entity->name = std::move(name);
		}

		const UUID& getUUID() const
		{
			return entity->uuid;
		}

		void keepOnlyComponentsWithIds(const std::vector<int>& ids)
		{
			entity->keepOnlyComponentsWithIds(ids, *world);
		}

		bool hasParent() const
		{
			return entity->getParent() != nullptr;
		}
		
		EntityRef getParent() const
		{
			return EntityRef(*entity->getParent(), *world);
		}

		std::optional<EntityRef> tryGetParent() const
		{
			const auto parent = entity->getParent();
			return parent != nullptr ? EntityRef(*parent, *world) : std::optional<EntityRef>();
		}

		void setParent(EntityRef& parent)
		{
			entity->setParent(parent.entity);
		}

		void setParent()
		{
			entity->setParent(nullptr);
		}

		const std::vector<Entity*>& getRawChildren() const
		{
			return entity->getChildren();
		}

		void addChild(EntityRef& child)
		{
			entity->addChild(*child.entity);
		}

		void detachChildren()
		{
			entity->detachChildren();
		}

		int8_t getHierarchyRevision() const
		{
			return entity->hierarchyRevision;
		}

		bool isValid() const
		{
			return entity != nullptr;
		}

	private:
		friend class World;

		Entity* entity = nullptr;
		World* world = nullptr;
	};
	
	class ConstEntityRef
	{
	public:
		ConstEntityRef() = default;
		ConstEntityRef(const ConstEntityRef& other) = default;
		ConstEntityRef(ConstEntityRef&& other) = default;

		ConstEntityRef& operator=(const ConstEntityRef& other) = default;
		ConstEntityRef& operator=(ConstEntityRef&& other) = default;

		ConstEntityRef(Entity& e, const World& w)
			: entity(&e)
			, world(&w)
		{}
		
		template <typename T>
		const T& getComponent() const
		{
			return entity->getComponent<T>();
		}

		template <typename T>
		const T* tryGetComponent() const
		{
			return entity->tryGetComponent<T>();
		}

		EntityId getEntityId() const
		{
			return entity->getEntityId();
		}

		template <typename T>
		bool hasComponent() const
		{
			return entity->hasComponent<T>();
		}

		const String& getName() const
		{
			return entity->name;
		}

		const UUID& getUUID() const
		{
			return entity->uuid;
		}

		bool hasParent() const
		{
			return entity->getParent() != nullptr;
		}

		ConstEntityRef getParent() const
		{
			return ConstEntityRef(*entity->getParent(), *world);
		}

		std::optional<ConstEntityRef> tryGetParent() const
		{
			const auto parent = entity->getParent();
			return parent != nullptr ? ConstEntityRef(*parent, *world) : std::optional<ConstEntityRef>();
		}

		[[deprecated]] const std::vector<Entity*>& getRawChildren() const
		{
			return entity->getChildren();
		}

		int8_t getHierarchyRevision() const
		{
			return entity->hierarchyRevision;
		}

	private:
		friend class World;

		Entity* entity;
		const World* world;
	};
}
