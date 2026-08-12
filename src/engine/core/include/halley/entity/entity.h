#pragma once

#include <array>
#include <memory>
#include "component.h"
#include "message.h"
#include "family_mask.h"
#include "entity_id.h"
#include "type_deleter.h"
#include <halley/data_structures/vector.h>

#include "prefab.h"
#include "halley/utils/type_traits.h"
#include "halley/maths/uuid.h"
#include <gsl/span>

#include "halley/utils/algorithm.h"

namespace Halley {
	class DataInterpolatorSet;
	class World;
	class System;
	class EntityRef;
	class Prefab;

	// True if T::onAddedToEntity(EntityRef&) exists
	template <class, class = std::void_t<>> struct HasOnAddedToEntityMember : std::false_type {};
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

	using WorldPartitionId = uint16_t;
	
	class Entity
	{
		friend class World;
		friend class System;
		friend class EntityRef;
		friend class ConstEntityRef;

	public:
		~Entity();

		template <typename T>
		T* tryGetComponent(bool evenIfDisabled = false)
		{
			constexpr uint32_t quickIndex = FamilyMask::RetrieveComponentIndex<T>::quickIndex;
			constexpr int id = FamilyMask::RetrieveComponentIndex<T>::componentIndex;
			
			if constexpr (quickIndex != 0) {
				if ((quickIndex & componentQuickMask) == 0) {
					return nullptr;
				}
			}
			
			if (evenIfDisabled || (enabled && parentEnabled)) [[likely]] {
				const auto& span = componentIds.span();
				const size_t n = liveComponents;
				for (size_t i = 0; i < n; ++i) {
					if (span[i] == id) {
						return static_cast<T*>(componentPtrs[i]);
					}
				}
			}
			return nullptr;
		}

		template <typename T>
		const T* tryGetComponent(bool evenIfDisabled = false) const
		{
			constexpr uint32_t quickIndex = FamilyMask::RetrieveComponentIndex<T>::quickIndex;
			constexpr int id = FamilyMask::RetrieveComponentIndex<T>::componentIndex;

			if constexpr (quickIndex != 0) {
				if ((quickIndex & componentQuickMask) == 0) {
					return nullptr;
				}
			}
			
			if (evenIfDisabled || (enabled && parentEnabled)) [[likely]] {
				const auto& span = componentIds.span();
				const size_t n = liveComponents;
				for (size_t i = 0; i < n; ++i) {
					if (span[i] == id) {
						return static_cast<const T*>(componentPtrs[i]);
					}
				}
			}
			return nullptr;
		}

		template <typename T>
		T& getComponent(bool evenIfDisabled = false)
		{
			auto value = tryGetComponent<T>(evenIfDisabled);
			if (value) {
				return *value;
			} else {
				throw Exception("Component " + String(typeid(T).name()) + " does not exist in entity.", HalleyExceptions::Entity);
			}
		}

		template <typename T>
		const T& getComponent(bool evenIfDisabled = false) const
		{
			auto value = tryGetComponent<T>(evenIfDisabled);
			if (value) {
				return *value;
			} else {
				throw Exception("Component " + String(typeid(T).name()) + " does not exist in entity.", HalleyExceptions::Entity);
			}
		}

		template <typename T>
		bool hasComponent(const World& world, bool evenIfDisabled = false) const
		{
			constexpr uint32_t quickIndex = FamilyMask::RetrieveComponentIndex<T>::quickIndex;
			constexpr int id = FamilyMask::RetrieveComponentIndex<T>::componentIndex;
			
			if constexpr (quickIndex != 0) {
				return (quickIndex & componentQuickMask) != 0;
			}
			
			if (evenIfDisabled || (enabled && parentEnabled)) [[likely]] {
				const auto& span = componentIds.span();
				const size_t n = liveComponents;
				for (size_t i = 0; i < n; ++i) {
					if (span[i] == id) {
						return true;
					}
				}
			}
			return false;
		}

		template <typename ... Ts>
		bool hasAnyComponent(const World& world, bool warnIfNoQuickIndexCoverage = false) const
		{
			constexpr auto quickIndices = getComponentQuickIndices<Ts...>();
			if constexpr (quickIndices.first != 0) {
				if constexpr (quickIndices.second) {
					// Full coverage, return true or false based purely on this
					return (quickIndices.first & componentQuickMask) != 0;
				} else {
					// Incomplete coverage - true is enough to satisfy, but not false
					if (warnIfNoQuickIndexCoverage) {
						Logger::logWarning("Components don't have quick index: " + getComponentsMissingQuickIndices<Ts...>(), true);
					}

					if ((quickIndices.first & componentQuickMask) != 0) {
						return true;
					}
				}
			}

			if (dirty) {
				return dirtyHasAnyComponents<Ts...>(world);
			} else {
				std::array<int, sizeof...(Ts)> result;
				auto span = gsl::span<int>(result.data(), result.size());
				getComponentIndices<Ts...>(result);
				return hasAnyBit(world, span);
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

		bool isEnabled() const
		{
			return enabled;
		}

		const UUID& getPrefabUUID() const
		{
			return prefabUUID;
		}

		const UUID& getInstanceUUID() const
		{
			return instanceUUID;
		}

		FamilyMaskType getMask() const;
		EntityId getEntityId() const;

		void refresh(MaskStorage* storage, ComponentDeleterTable& table, gsl::span<const int> alwaysEnabledComponents);
		
		void sortChildrenByInstanceUUIDs(const Vector<UUID>& uuids);

		bool isEmpty() const;

		bool isNetworkOwner(const World& world) const;
		bool isNetworkAuthority(const World& world) const;

		int getParentingDepth() const;

		const String& getEnableRules() const;
		void setEnableRules(String rules);

	private:
		// !!! WARNING !!!
		// The order of elements in this class was carefully chosen to maximise cache performance!
		// Be SURE to verify that no performance-critical fields get bumped to a worse cacheline if you change anything here

		// Cacheline 0
		Vector<int16_t> componentIds;
		Vector<Component*> componentPtrs;
		uint8_t liveComponents = 0;
		bool dirty : 1;
		bool alive : 1;
		bool serializable : 1;
		bool reloaded : 1;
		bool enabled : 1;
		bool parentEnabled : 1;
		bool selectable : 1;
		bool fromNetwork : 1;
		
		uint8_t childrenRevision = 0;
		uint8_t hierarchyRevision = 0;
		uint8_t componentRevision = 0;
		WorldPartitionId worldPartition = 0;
		uint32_t componentQuickMask = 0;

		FamilyMaskType mask;
		Entity* parent = nullptr;
		EntityId entityId;

		// Cacheline 1
		Vector<Entity*> children;
		UUID instanceUUID;
		UUID prefabUUID;
		std::shared_ptr<const Prefab> prefab;

		// Cacheline 2
		std::unique_ptr<String> name;
		std::unique_ptr<String> enableRules;
		uint32_t lastFrameModified = 0;

		Entity();
		void destroyComponents(ComponentDeleterTable& storage);

		template <typename T>
		Entity& addComponent(World& world, T* component)
		{
			auto& deleterTable = getComponentDeleterTable(world);
			addComponent(component, T::componentIndex, T::quickIndex, T::componentName, deleterTable);
			TypeDeleter<T>::initialize(deleterTable);

			markDirty(world);
			return *this;
		}

		template <typename T>
		Entity& removeComponent(World& world)
		{
			removeComponentById(world, T::componentIndex, T::quickIndex);
			return *this;
		}

		void addComponent(Component* component, int id, uint32_t quickIndex, const char* componentName, ComponentDeleterTable& deleterTable);
		void removeComponentAt(int index, uint32_t quickIndex);
		void removeComponentById(World& world, int id);
		void removeComponentById(World& world, int id, uint32_t quickIndex);
		void removeAllComponents(World& world);
		void deleteComponent(Component* component, int id, ComponentDeleterTable& table);
		void setEnabled(World& world, bool enabled);

		void onReady();

		void markDirty(World& world);
		ComponentDeleterTable& getComponentDeleterTable(World& world);

		Entity* getParent() const { return parent; }
		void setParent(Entity* parent, bool propagate = true, size_t childIdx = -1);
		void clearParentAndChildren();
		const Vector<Entity*>& getChildren() const { return children; }
		void addChild(Entity& child);
		void detachChildren();
		void markHierarchyDirty();
		void propagateChildrenChange();
		void propagateChildWorldPartition(WorldPartitionId newWorldPartition);
		void propagateEnabled(bool enabled, bool parentEnabled);

		DataInterpolatorSet& setupNetwork(EntityRef& ref, uint8_t peerId);
		std::optional<uint8_t> getOwnerPeerId() const;
        std::optional<uint8_t> getAuthorityPeerId() const;
		void setFromNetwork(bool fromNetwork);

		void destroy(World& world);
		void doDestroy(World& world, bool updateParenting);

		bool hasBit(const World& world, int index) const;
		bool hasAnyBit(const World& world, gsl::span<const int> indices) const;
		
		template <typename T, typename ... Ts>
		constexpr static void getComponentIndices(gsl::span<int> dst)
		{
			dst[0] = FamilyMask::RetrieveComponentIndex<T>::componentIndex;
			if constexpr (sizeof...(Ts) > 0) {
				getComponentIndices<Ts...>(dst.subspan(1));
			}
		}

		template <typename T, typename ... Ts>
		constexpr static std::pair<uint32_t, bool> getComponentQuickIndices()
		{
			constexpr auto quickIdx = FamilyMask::RetrieveComponentIndex<T>::quickIndex;
			constexpr bool coverage = quickIdx != 0;

			if constexpr (sizeof...(Ts) > 0) {
				auto [otherIdx, otherCoverage] = getComponentQuickIndices<Ts...>();
				return { quickIdx | otherIdx, coverage && otherCoverage };
			} else {
				return { quickIdx, coverage };
			}
		}

		template <typename T, typename ... Ts>
		static String getComponentsMissingQuickIndices()
		{
			constexpr auto quickIdx = FamilyMask::RetrieveComponentIndex<T>::quickIndex;
			constexpr bool coverage = quickIdx != 0;
			constexpr auto myName = coverage ? std::string_view(typeid(T).name()) : "";

			if constexpr (sizeof...(Ts) > 0) {
				auto rest = getComponentsMissingQuickIndices<Ts...>();
				if (rest.isEmpty()) {
					return myName;
				} else {
					return String(myName) + ", " + rest;
				}
			} else {
				return myName;
			}
		}

		template <typename T, typename ... Ts>
		bool dirtyHasAnyComponents(const World& world) const
		{
			if (tryGetComponent<T>() != nullptr) {
				return true;
			}
			if constexpr (sizeof...(Ts) > 0) {
				return dirtyHasAnyComponents<Ts...>(world);
			} else {
				return false;
			}
		}

		size_t getMemoryUsage(ComponentDeleterTable& table) const;
	};

	//static_assert(sizeof(Entity) <= 128); // We'll lose some significant perf due to going over two cache lines

	class EntityRef;
	
	class EntityRefIterable {
	public:
		class Iterator {
		public:

			constexpr Iterator(Vector<Entity*>::const_iterator iter, World& world)
				: iter(iter)
				, world(world)
			{}

			constexpr Iterator& operator++()
			{
				++iter;
				return *this;
			}

			constexpr bool operator==(const Iterator& other) const
			{
				return iter == other.iter;
			}

			constexpr bool operator!=(const Iterator& other) const
			{
				return iter != other.iter;
			}

			EntityRef operator*() const;

		private:
			Vector<Entity*>::const_iterator iter;
			World& world;
		};
		
		constexpr EntityRefIterable(const Vector<Entity*>& entities, World& world)
			: entities(entities)
			, world(world)
		{}

		constexpr Iterator begin() const
		{
			return Iterator(entities.begin(), world);
		}

		constexpr Iterator end() const
		{
			return Iterator(entities.end(), world);
		}

	private:
		const Vector<Entity*>& entities;
		World& world;
	};

	class ConstEntityRef;

	class EntityRef
	{
		friend class ConstEntityRef;
	
	public:
		EntityRef() = default;
		EntityRef(const EntityRef& other) = default;
		EntityRef(EntityRef&& other) noexcept = default;

		EntityRef& operator=(const EntityRef& other) = default;
		EntityRef& operator=(EntityRef&& other) noexcept = default;

		EntityRef(Entity& e, World& w)
			: entity(&e)
			, world(&w)
		{
#ifdef _DEBUG
			entityId = entity->getEntityId();
#endif
		}

		~EntityRef() = default;

		template <typename T>
		EntityRef& addComponent(T&& component)
		{
			validateComponentType<T>();
			validate();
			
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
			validateComponentType<T>();
			validate();
			entity->removeComponent<T>(*world);
			return *this;
		}

		template <typename T>
		EntityRef& tryAddComponent(T&& component)
		{
			if (!hasComponent<T>()) {
				addComponent<T>(std::move(component));
			}
			return *this;
		}

		template <typename T>
		EntityRef& tryRemoveComponent()
		{
			if (hasComponent<T>()) {
				removeComponent<T>();
			}
			return *this;
		}

		EntityRef& removeComponentById(int id)
		{
			validate();
			entity->removeComponentById(*world, id);
			return *this;
		}

		EntityRef& removeAllComponents()
		{
			validate();
			entity->removeAllComponents(*world);
			return *this;
		}

		template <typename T>
		T& getComponent(bool evenIfDisabled = false)
		{
			validateComponentType<T>();
			validate();
			return entity->getComponent<T>(evenIfDisabled);
		}

		template <typename T>
		const T& getComponent(bool evenIfDisabled = false) const
		{
			validateComponentType<T>();
			validate();
			return entity->getComponent<T>(evenIfDisabled);
		}

		template <typename T>
		T* tryGetComponent(bool evenIfDisabled = false)
		{
			validateComponentType<T>();
			validate();
			return entity->tryGetComponent<T>(evenIfDisabled);
		}

		template <typename T>
		const T* tryGetComponent(bool evenIfDisabled = false) const
		{
			validateComponentType<T>();
			validate();
			return entity->tryGetComponent<T>(evenIfDisabled);
		}

		template <typename T>
		T& getOrAddComponent()
		{
			validateComponentType<T>();
			validate();
			if (auto* existing = entity->tryGetComponent<T>(true)) {
				return *existing;
			} else {
				addComponent(T());
				return getComponent<T>(true);
			}
		}

		template <typename T>
		T* tryGetComponentInAncestors()
		{
			validateComponentType<T>();
			auto* c = tryGetComponent<T>();
			if (c) {
				return c;
			}
			if (auto parent = getParent(); parent.isValid()) {
				return parent.tryGetComponentInAncestors<T>();
			}
			return nullptr;
		}

		template <typename T>
		const T* tryGetComponentInAncestors() const
		{
			validateComponentType<T>();
			auto* c = tryGetComponent<T>();
			if (c) {
				return c;
			}
			if (const auto parent = getParent(); parent.isValid()) {
				return parent.tryGetComponentInAncestors<T>();
			}
			return nullptr;
		}

		template <typename T>
		EntityId tryGetEntityIdWithComponentInAncestors() const
		{
			validateComponentType<T>();
			auto* comp = tryGetComponent<T>();
			if (comp) {
				return getEntityId();
			}
			if (const auto parent = getParent(); parent.isValid()) {
				const auto parentId = parent.tryGetEntityIdWithComponentInAncestors<T>();
				if (parentId.isValid()) {
					return parentId;
				}
			}
			return {};
		}

		template <typename T>
		T* tryGetComponentInTree()
		{
			validateComponentType<T>();
			auto* comp = tryGetComponent<T>();
			if (comp) {
				return comp;
			}
			for (auto& child: getRawChildren()) {
				auto* childComp = EntityRef(*child, getWorld()).tryGetComponentInTree<T>();
				if (childComp) {
					return childComp;
				}
			}
			return nullptr;
		}

		template <typename T>
		const T* tryGetComponentInTree() const
		{
			validateComponentType<T>();
			auto* comp = tryGetComponent<T>();
			if (comp) {
				return comp;
			}
			for (auto& child: getRawChildren()) {
				auto* childComp = EntityRef(*child, getWorld()).tryGetComponentInTree<T>();
				if (childComp) {
					return childComp;
				}
			}
			return nullptr;
		}

		template <typename T>
		EntityId tryGetEntityIdWithComponentInTree() const
		{
			validateComponentType<T>();
			auto* comp = tryGetComponent<T>();
			if (comp) {
				return getEntityId();
			}
			for (auto& child : getRawChildren()) {
				auto childId = EntityRef(*child, getWorld()).tryGetEntityIdWithComponentInTree<T>();
				if (childId.isValid()) {
					return childId;
				}
			}
			return {};
		}

		template <typename T>
		size_t tryGetEntityIdsWithComponentInTree(Vector<EntityId>& entityIds) const
		{
			validateComponentType<T>();
			size_t count = 0;
			auto* comp = tryGetComponent<T>();
			if (comp) {
				entityIds.emplace_back(getEntityId());
				++count;
			}
			for (auto& child : getRawChildren()) {
				auto childId = EntityRef(*child, getWorld());
				if (childId.isValid()) {
					count += childId.tryGetEntityIdsWithComponentInTree<T>(entityIds);
				}
			}
			return count;
		}

		EntityId getEntityId() const
		{
			if (!entity) {
				return EntityId();
			}
			validate();
			return entity->getEntityId();
		}

		template <typename T>
		bool hasComponent(bool evenIfDisabled = false) const
		{
			validateComponentType<T>();
			validate();
			return entity->hasComponent<T>(*world, evenIfDisabled);
		}

		template <typename ... Ts>
		bool hasAnyComponent(bool warnIfNoQuickIndexCoverage = false) const
		{
			validate();
			return entity->hasAnyComponent<Ts...>(*world, warnIfNoQuickIndexCoverage);
		}

		template <typename T>
		bool hasComponentInTree() const
		{
			validateComponentType<T>();
			if (hasComponent<T>()) {
				return true;
			}
			for (auto& c: getRawChildren()) {
				if (EntityRef(*c, getWorld()).hasComponentInTree<T>()) {
					return true;
				}
			}
			return false;
		}

		template <typename T>
		bool hasComponentInAncestors() const
		{
			validateComponentType<T>();
			if (hasComponent<T>()) {
				return true;
			}
			if (const auto parent = getParent(); parent.isValid()) {
				return parent.hasComponentInAncestors<T>();
			}
			return false;
		}

		bool hasEntityIdInAncestors(EntityId parentId) const
		{
			if (!parentId.isValid()) {
				return false;
			}
			validate();
			for (auto parent = getParent(); parent.isValid(); parent = parent.getParent()) {
				if (parent.getEntityId() == parentId) {
					return true;
				}
			}

			return false;
		}

		const String& getName() const
		{
			validate();
			return entity->name ? *entity->name : String::emptyString();
		}

		void setName(String name)
		{
			validate();
			if (entity->name) {
				*entity->name = std::move(name);
			} else {
				entity->name = std::make_unique<String>(std::move(name));
			}
		}

		const String& getEnableRules() const
		{
			validate();
			return entity->getEnableRules();
		}

		void setEnableRules(String enableRules)
		{
			validate();
			entity->setEnableRules(std::move(enableRules));
		}

		const UUID& getInstanceUUID() const
		{
			validate();
			return entity->instanceUUID;
		}

		const UUID& getPrefabUUID() const
		{
			validate();
			return entity->prefabUUID;
		}

		bool hasParent() const
		{
			validate();
			return entity->getParent() != nullptr;
		}
		
		EntityRef getParent() const
		{
			validate();
			auto parent = entity->getParent();
			return parent ? EntityRef(*parent, *world) : EntityRef();
		}

		std::optional<EntityRef> tryGetParent() const
		{
			validate();
			const auto parent = entity->getParent();
			return parent != nullptr ? EntityRef(*parent, *world) : std::optional<EntityRef>();
		}

		void setParent(const EntityRef& parent, size_t childIdx = -1)
		{
			validate();
			entity->setParent(parent.entity, true, childIdx);
		}

		void setParent()
		{
			validate();
			entity->setParent(nullptr);
		}

		const Vector<Entity*>& getRawChildren() const
		{
			validate();
			return entity->getChildren();
		}

		EntityRefIterable getChildren() const
		{
			validate();
			return EntityRefIterable(entity->getChildren(), *world);
		}

		EntityRef getChildWithName(const String& name) const
		{
			validate();

			for (const auto child : getChildren()) {
				if (child.getName() == name) {
					return child;
				}
			}

			return {};
		}

		bool hasChildren() const
		{
			validate();
			return !entity->getChildren().empty();
		}

		void addChild(EntityRef& child)
		{
			validate();
			entity->addChild(*child.entity);
		}

		void detachChildren()
		{
			validate();
			entity->detachChildren();
		}

		uint8_t getHierarchyRevision() const
		{
			validate();
			return entity->hierarchyRevision;
		}

		uint8_t getComponentRevision() const
		{
			validate();
			return entity->componentRevision;
		}

		uint8_t getChildrenRevision() const
		{
			validate();
			return entity->childrenRevision;
		}

		WorldPartitionId getWorldPartition() const
		{
			validate();
			return entity->worldPartition;
		}

		bool isValid() const
		{
			return entity != nullptr && world != nullptr;
		}

		bool isAlive() const
		{
			validate();
			return entity->isAlive();
		}

		bool isSelectable() const
		{
			validate();
			return entity->selectable;
		}

		void setSelectable(bool selectable)
		{
			validate();
			entity->selectable = selectable;
		}

		bool isEnabled() const
		{
			validate();
			return entity->isEnabled();
		}

		void setEnabled(bool enabled)
		{
			validate();
			entity->setEnabled(*world, enabled);
		}

		bool operator==(const EntityRef& other) const
		{
			return entity == other.entity && world == other.world
#ifdef _DEBUG
			&& entityId == other.entityId
#endif
			;
		}

		bool operator!=(const EntityRef& other) const
		{
			return !(*this == other);
		}

		bool operator<(const EntityRef& other) const
		{
			return entity < other.entity;
		}

		int getParentingDepth() const
		{
			validate();
			return entity->getParentingDepth();
		}

		World& getWorld() const
		{
			validate();
			return *world;
		}

		size_t getNumComponents() const
		{
			validate();
			return static_cast<size_t>(entity->liveComponents);
		}

		EntityRef& setSerializable(bool serializable)
		{
			validate();
			entity->serializable = serializable;
			return *this;
		}

		bool isSerializable() const
		{
			validate();
			return entity->serializable;
		}

		void setReloaded();

		bool wasReloaded()
		{
			validate();
			return entity->reloaded;
		}

		void sortChildrenByInstanceUUIDs(const Vector<UUID>& uuids)
		{
			validate();
			entity->sortChildrenByInstanceUUIDs(uuids);
		}

		void setPrefab(std::shared_ptr<const Prefab> prefab, UUID prefabUUID)
		{
			validate();
			HalleyAssertDev(!prefab || prefabUUID.isValid());
			entity->prefab = std::move(prefab);
			entity->prefabUUID = prefabUUID;
		}

		const std::shared_ptr<const Prefab>& getPrefab() const
		{
			validate();
			return entity->prefab;
		}

		std::optional<String> getPrefabAssetId() const
		{
			return entity && entity->prefab ? entity->prefab->getAssetId() : std::optional<String>{};
		}

		DataInterpolatorSet& setupNetwork(uint8_t peerId)
		{
			validate();
			return entity->setupNetwork(*this, peerId);
		}

		std::optional<uint8_t> getOwnerPeerId() const
		{
			validate();
			return entity->getOwnerPeerId();
		}

        std::optional<uint8_t> getAuthorityPeerId() const
        {
            validate();
            return entity->getAuthorityPeerId();
        }

		bool isLocal() const
		{
			validate();
			return entity->isNetworkOwner(*world);
		}

		bool isNetworkOwner() const
		{
			validate();
			return entity->isNetworkOwner(*world);
		}

		bool isNetworkAuthority() const
		{
			validate();
			return entity->isNetworkAuthority(*world);
		}

		void setFromNetwork(bool fromNetwork)
		{
			validate();
			entity->setFromNetwork(fromNetwork);
		}

		uint32_t getLastFrameModified() const
		{
			validate();
			return entity->lastFrameModified;
		}

		void setModifiedThisFrame(bool checkAnchestors = false);

		void setLastFrameModified(uint32_t frame)
		{
			validate();
			entity->lastFrameModified = frame;
		}

		bool isEmpty() const
		{
			return !entity || entity->isEmpty();
		}

		void validate() const
		{
			HalleyAssertDebug(isValid());
#ifdef _DEBUG
			HalleyAssertDebug(entity->getEntityId() == entityId);
#endif
		}

		gsl::span<const int16_t> getComponentIds() const
		{
			return entity->componentIds.const_span().subspan(0, entity->liveComponents);
		}

		gsl::span<const Component* const> getComponentPtrs() const
		{
			return entity->componentPtrs.const_span().subspan(0, entity->liveComponents);
		}

	private:
		friend class World;

		Entity* entity = nullptr;
		World* world = nullptr;

#ifdef _DEBUG
		EntityId entityId;
#endif

		template <typename T>
		constexpr static void validateComponentType()
		{
			static_assert(!std::is_pointer<T>::value, "Cannot pass pointer to component");
			static_assert(!std::is_same<T, Component>::value, "Cannot add base class Component to entity, make sure type isn't being erased");
			static_assert(std::is_base_of<Component, T>::value, "Components must extend the Component class");
			static_assert(!std::is_polymorphic<T>::value, "Components cannot be polymorphic (i.e. they can't have virtual methods)");
			static_assert(std::is_default_constructible<T>::value, "Components must have a default constructor");
		}
	};
	
	class ConstEntityRef
	{
	public:
		ConstEntityRef() = default;
		ConstEntityRef(const ConstEntityRef& other) = default;
		ConstEntityRef(ConstEntityRef&& other) = default;

		ConstEntityRef& operator=(const ConstEntityRef& other) = default;
		ConstEntityRef& operator=(ConstEntityRef&& other) = default;

		constexpr ConstEntityRef(const Entity& e, const World& w)
			: entity(&e)
			, world(&w)
		{}

		constexpr ConstEntityRef(const EntityRef& e)
			: entity(e.entity)
			, world(e.world)
		{}
		
		template <typename T>
		const T& getComponent() const
		{
			return entity->getComponent<T>();
		}

		template <typename T>
		const T* tryGetComponent(bool evenIfDisabled = false) const
		{
			return entity->tryGetComponent<T>(evenIfDisabled);
		}

		EntityId getEntityId() const
		{
			return entity->getEntityId();
		}

		template <typename T>
		bool hasComponent() const
		{
			return entity->hasComponent<T>(*world);
		}

		const String& getName() const
		{
			return entity->name ? *entity->name : String::emptyString();
		}

		const UUID& getInstanceUUID() const
		{
			return entity->instanceUUID;
		}

		const UUID& getPrefabUUID() const
		{
			return entity->prefabUUID;
		}

		bool hasParent() const
		{
			return entity->getParent() != nullptr;
		}

		ConstEntityRef getParent() const
		{
			auto parent = entity->getParent();
			return parent != nullptr ? ConstEntityRef(*parent, *world) : ConstEntityRef();
		}

		std::optional<ConstEntityRef> tryGetParent() const
		{
			const auto parent = entity->getParent();
			return parent != nullptr ? ConstEntityRef(*parent, *world) : std::optional<ConstEntityRef>();
		}

		const Vector<Entity*>& getRawChildren() const
		{
			return entity->getChildren();
		}

		uint8_t getHierarchyRevision() const
		{
			return entity->hierarchyRevision;
		}

		uint8_t getComponentRevision() const
		{
			return entity->componentRevision;
		}

		uint8_t getChildrenRevision() const
		{
			return entity->childrenRevision;
		}

		size_t getNumComponents() const
		{
			HalleyAssertDev(entity);
			return size_t(entity->liveComponents);
		}

		bool isSerializable() const
		{
			HalleyAssertDev(entity);
			return entity->serializable;
		}

		bool isValid() const
		{
			return entity != nullptr;
		}

		std::optional<uint8_t> getOwnerPeerId() const
		{
			HalleyAssertDev(entity);
			return entity->getOwnerPeerId();
		}

        std::optional<uint8_t> getAuthorityPeerId() const
        {
            HalleyAssertDev(entity);
            return entity->getAuthorityPeerId();
        }

		bool isLocal() const
		{
			HalleyAssertDev(entity);
			return entity->isNetworkOwner(*world);
		}

		bool isNetworkOwner() const
		{
			HalleyAssertDev(entity);
			return entity->isNetworkOwner(*world);
		}

		bool isNetworkAuthority() const
		{
			HalleyAssertDev(entity);
			return entity->isNetworkAuthority(*world);
		}

		template <typename T>
		const T* tryGetComponentInAncestors() const
		{
			auto* c = tryGetComponent<T>();
			if (c) {
				return c;
			}
			if (const auto parent = getParent(); parent.isValid()) {
				return parent.tryGetComponentInAncestors<T>();
			}
			return nullptr;
		}

		template <typename T>
		const T* tryGetComponentInTree() const
		{
			auto* comp = tryGetComponent<T>();
			if (comp) {
				return comp;
			}
			for (auto& child: getRawChildren()) {
				auto* childComp = ConstEntityRef(*child, *world).tryGetComponentInTree<T>();
				if (childComp) {
					return childComp;
				}
			}
			return nullptr;
		}

		WorldPartitionId getWorldPartition() const
		{
			return entity->worldPartition;
		}

		uint32_t getLastFrameModified() const
		{
			return entity->lastFrameModified;
		}

		gsl::span<const int16_t> getComponentIds() const
		{
			return entity->componentIds.const_span().subspan(0, entity->liveComponents);
		}

		gsl::span<const Component* const> getComponentPtrs() const
		{
			return entity->componentPtrs.const_span().subspan(0, entity->liveComponents);
		}

	private:
		friend class World;

		const Entity* entity;
		const World* world;
	};

	inline EntityRef EntityRefIterable::Iterator::operator*() const
	{
		return EntityRef(*(*iter), world);
	}
	
}
