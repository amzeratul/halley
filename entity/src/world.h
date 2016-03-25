#pragma once

#include <memory>
#include <vector>
#include "entity_id.h"
#include "family_mask.h"
#include "family.h"

namespace Halley {
	class Entity;
	class System;

	class World
	{
	public:
		World();
		~World();

		void step(Time elapsed);

		System& addSystemName(String name);
		System& addSystem(std::unique_ptr<System> system);
		void removeSystem(System& system);
		std::vector<System*> getSystems();
		System& getSystem(String name);

		Entity& createEntity();
		void destroyEntity(EntityId id);
		Entity& getEntity(EntityId id);
		Entity* tryGetEntity(EntityId id);
		size_t numEntities() const;

		template <typename T>
		Family& getFamily()
		{
			FamilyMask::Type mask = T::Type::readMask;
			auto iter = families.find(mask);
			if (iter != families.end()) {
				return *iter->second;
			}

			auto newFam = std::make_unique<FamilyImpl<T>>();
			Family* newFamPtr = newFam.get();
			onAddFamily(*newFamPtr);
			families[mask] = std::move(newFam);
			return *newFamPtr;
		}
		
	private:
		std::vector<std::unique_ptr<System>> systems;
		bool systemsDirty = false;

		EntityId nextUid = 0;
		std::vector<Entity*> entities;
		std::vector<Entity*> entitiesPendingCreation;
		MappedPool<Entity*> entityMap;

		std::map<FamilyMask::Type, std::unique_ptr<Family>> families;
		//std::shared_ptr<EntityRegistry> registry;

		void spawnPending();
		void updateEntities();
		void deleteEntity(Entity* entity);

		void updateSystems(Time elapsed);
		void doAddSystem(System* system, String name);
		void onAddFamily(Family& family);
	};
}
