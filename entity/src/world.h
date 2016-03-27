#pragma once

#include <memory>
#include <vector>
#include "entity_id.h"
#include "family_mask.h"
#include "family.h"
#include <allocators>
#include <chrono>

namespace Halley {
	class Entity;
	class System;

	class World
	{
	public:
		World();
		~World();

		bool hasSystemsOnTimeLine(TimeLine timeline) const;
		void step(TimeLine timeline, Time elapsed);
		double getLastStepLength() const { return lastStepLength; }

		System& addSystemByName(String name, TimeLine timeline);
		System& addSystem(std::unique_ptr<System> system, TimeLine timeline);
		void removeSystem(System& system);
		std::vector<System*> getSystems();
		System& getSystem(String name);

		EntityRef createEntity();
		void destroyEntity(EntityId id);
		EntityRef getEntity(EntityId id);
		Entity* tryGetEntity(EntityId id);
		size_t numEntities() const;

		void onEntityDirty();

		template <typename T>
		Family& getFamily()
		{
			FamilyMaskType mask = T::Type::readMask();
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
		std::array<std::vector<std::unique_ptr<System>>, static_cast<int>(TimeLine::NUMBER_OF_TIMELINES)> systems;
		bool entityDirty = false;

		double lastStepLength;

		EntityId nextUid = 0;
		std::vector<Entity*> entities;
		std::vector<Entity*> entitiesPendingCreation;
		MappedPool<Entity*> entityMap;

		std::map<FamilyMaskType, std::unique_ptr<Family>> families;
		//std::shared_ptr<EntityRegistry> registry;

		void allocateEntity(Entity* entity);
		void spawnPending();
		void updateEntities();
		void deleteEntity(Entity* entity);

		std::vector<std::unique_ptr<System>>& getSystems(TimeLine timeline);
		const std::vector<std::unique_ptr<System>>& getSystems(TimeLine timeline) const;
		void updateSystems(TimeLine timeline, Time elapsed);
		void onAddFamily(Family& family);
	};
}
