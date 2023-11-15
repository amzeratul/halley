#pragma once

#include <memory>
#include <typeindex>
#include <typeinfo>
#include <type_traits>
#include "entity_id.h"
#include "family_mask.h"
#include "family.h"
#include <halley/time/halleytime.h>
#include <halley/text/halleystring.h>
#include <halley/data_structures/mapped_pool.h>
#include <halley/time/stopwatch.h>
#include <halley/data_structures/vector.h>
#include <halley/data_structures/tree_map.h>
#include "service.h"
#include "create_functions.h"
#include "halley/utils/attributes.h"
#include <halley/data_structures/memory_pool.h>
#include "system_message.h"
#include "world_reflection.h"
#include "system_interface.h"

namespace Halley {
	class SystemMessage;
	enum class SystemMessageDestination;
	struct SystemMessageContext;
	class UUID;
	class ConfigNode;
	class RenderContext;
	class Entity;
	class System;
	class Painter;
	class HalleyAPI;

	class IWorldNetworkInterface {
	public:
		virtual ~IWorldNetworkInterface() = default;

		virtual bool isRemote(ConstEntityRef entity) const = 0;
		virtual void sendEntityMessage(EntityRef entity, int messageId, Bytes messageData) = 0;
		virtual void sendSystemMessage(String targetSystem, int messageId, Bytes messageData, SystemMessageDestination destination, SystemMessageCallback callback) = 0;
		virtual bool isHost() = 0;
	};

	class World
	{
	public:
		World(const HalleyAPI& api, Resources& resources, WorldReflection reflection);
		~World();

		static std::unique_ptr<World> make(const HalleyAPI& api, Resources& resources, const String& sceneName, bool devMode);

		void step(TimeLine timeline, Time elapsed);
		void render(RenderContext& rc);
		bool hasSystemsOnTimeLine(TimeLine timeline) const;
		
		System& addSystem(std::unique_ptr<System> system, TimeLine timeline);
		void removeSystem(System& system);
		Vector<System*> getSystems();
		System& getSystem(const String& name);
		Vector<std::unique_ptr<System>>& getSystems(TimeLine timeline);
		const Vector<std::unique_ptr<System>>& getSystems(TimeLine timeline) const;

		Service& addService(std::shared_ptr<Service> service);
		void loadSystems(const ConfigNode& config);
		
		template <typename T>
		T* tryGetService(std::string_view systemName = "")
		{
			static_assert(std::is_base_of<Service, T>::value, "Must extend Service");

			const auto serviceName = typeid(T).name();
			const auto rawService = doTryGetService(serviceName);
			if (!rawService) {
				if constexpr (std::is_default_constructible_v<T>) {
					return dynamic_cast<T*>(&addService(std::make_shared<T>()));
				} else {
					return nullptr;
				}
			}
			return dynamic_cast<T*>(rawService);
		}
		
		template <typename T>
		T& getService(std::string_view systemName = "")
		{
			if (T* service = tryGetService<T>(systemName)) {
				return *service;
			}
			const auto serviceName = typeid(T).name();
			throw Exception(String("Service \"") + serviceName + "\" required by \"" + (systemName.empty() ? "" : (String(systemName) + "System")) + "\" not found, and it cannot be default constructed.", HalleyExceptions::Entity);
		}

		EntityRef createEntity(String name = "", std::optional<EntityRef> parent = {});
		EntityRef createEntity(String name, EntityId parentId);
		EntityRef createEntity(UUID uuid, String name, EntityId parentId);
		EntityRef createEntity(UUID uuid, String name = "", std::optional<EntityRef> parent = {}, uint8_t worldPartition = 0);

		void destroyEntity(EntityId id);
		void destroyEntity(EntityRef entity);

		EntityRef getEntity(EntityId id);
		ConstEntityRef getEntity(EntityId id) const;
		EntityRef tryGetEntity(EntityId id);
		ConstEntityRef tryGetEntity(EntityId id) const;
		Entity* tryGetRawEntity(EntityId id);
		const Entity* tryGetRawEntity(EntityId id) const;
		std::optional<EntityRef> findEntity(const UUID& id, bool includePending = false);

		size_t numEntities() const;
		Vector<EntityRef> getEntities();
		Vector<ConstEntityRef> getEntities() const;
		Vector<EntityRef> getTopLevelEntities();
		Vector<ConstEntityRef> getTopLevelEntities() const;

		void spawnPending(); // Warning: use with care, will invalidate entities

		void onEntityDirty();

		void setEntityReloaded();

		template <typename T>
		Family& getFamily() noexcept
		{
			// Disable re-using of families due to optional components messing them up
			/*
			FamilyMaskType mask = T::Type::readMask();
			auto iter = families.find(mask);
			if (iter != families.end()) {
				return *iter->second;
			}
			*/

			return addFamily(std::make_unique<FamilyImpl<T>>(*maskStorage));
		}

		const WorldReflection& getReflection() const;

		MaskStorage& getMaskStorage() const noexcept;
		ComponentDeleterTable& getComponentDeleterTable();

		size_t sendSystemMessage(SystemMessageContext context, const String& targetSystem, SystemMessageDestination destination);

		void setNetworkInterface(IWorldNetworkInterface* interface);
		bool isEntityNetworkRemote(EntityId entityId) const;
		bool isEntityNetworkRemote(EntityRef entity) const;
		bool isEntityNetworkRemote(ConstEntityRef entity) const;
		void sendNetworkMessage(EntityId entityId, int messageId, std::unique_ptr<Message> msg);
		void sendNetworkSystemMessage(const String& targetSystem, const SystemMessageContext& context, SystemMessageDestination destination);
		std::unique_ptr<Message> deserializeMessage(int msgId, gsl::span<const std::byte> data);
		std::unique_ptr<Message> deserializeMessage(const String& messageName, const ConfigNode& data);
		std::unique_ptr<SystemMessage> deserializeSystemMessage(int msgId, gsl::span<const std::byte> data);
		std::unique_ptr<SystemMessage> deserializeSystemMessage(const String& messageName, const ConfigNode& data);

		bool isDevMode() const;

		void setEditor(bool isEditor);
		bool isEditor() const;

		void onEntityDestroyed(const UUID& uuid);
		bool isTerminating() const;

		float getTransform2DAnisotropy() const;
		void setTransform2DAnisotropy(float anisotropy);

		template <typename T>
		T& getInterface()
		{
			const auto iter = systemInterfaces.find(std::type_index(typeid(T)));
			if (iter == systemInterfaces.end()) {
				throw Exception(String("World does not have system interface \"") + typeid(T).name() + "\"", HalleyExceptions::Scripting);
			}
			return dynamic_cast<T&>(*iter->second);
		}

		template <typename T>
		void setInterface(T* interface)
		{
			static_assert(std::is_abstract_v<T>);
			static_assert(std::is_base_of_v<ISystemInterface, T>);
			systemInterfaces[std::type_index(typeid(T))] = interface;
		}

		bool isHeadless() const;
		void setHeadless(bool headless);

	private:
		const HalleyAPI& api;
		Resources& resources;
		std::array<Vector<std::unique_ptr<System>>, static_cast<int>(TimeLine::NUMBER_OF_TIMELINES)> systems;
		WorldReflection reflection;
		bool entityDirty = false;
		bool entityReloaded = false;
		bool editor = false;
		bool devMode = false;
		bool terminating = false;
		bool headless = false;
		
		Vector<Entity*> entities;
		Vector<Entity*> entitiesPendingCreation;
		MappedPool<Entity*> entityMap;
		HashMap<UUID, Entity*> uuidMap;

		//TreeMap<FamilyMaskType, std::unique_ptr<Family>> families;
		Vector<std::unique_ptr<Family>> families;
		TreeMap<String, std::shared_ptr<Service>> services;

		TreeMap<FamilyMaskType, Vector<Family*>> familyCache;

		std::shared_ptr<MaskStorage> maskStorage;
		std::shared_ptr<ComponentDeleterTable> componentDeleterTable;
		std::shared_ptr<PoolAllocator<Entity>> entityPool;

		std::list<SystemMessageContext> pendingSystemMessages;
		
		IWorldNetworkInterface* networkInterface = nullptr;
		float transform2DAnisotropy = 1.0f;

    	HashMap<std::type_index, ISystemInterface*> systemInterfaces;

		void allocateEntity(Entity* entity);
		void updateEntities();
		void initSystems(gsl::span<const TimeLine> timelines);

		void doDestroyEntity(EntityId id);
		void doDestroyEntity(Entity* entity);
		void deleteEntity(Entity* entity);

		void updateSystems(TimeLine timeline, Time elapsed);
		void renderSystems(RenderContext& rc) const;

		NOINLINE Family& addFamily(std::unique_ptr<Family> family) noexcept;
		void onAddFamily(Family& family) noexcept;

		Service* doTryGetService(const String& name) const;

		const Vector<Family*>& getFamiliesFor(const FamilyMaskType& mask);

		void processSystemMessages(TimeLine timeline);
	};
}
