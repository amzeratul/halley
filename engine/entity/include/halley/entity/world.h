#pragma once

#include <memory>
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

namespace Halley {
	class Entity;
	class System;
	class Painter;
	class HalleyAPI;

	class World
	{
	public:
		World(HalleyAPI* api);
		~World();

		void step(TimeLine timeline, Time elapsed);
		void render(Painter& painter) const;
		bool hasSystemsOnTimeLine(TimeLine timeline) const;
		
		long long getAverageTime(TimeLine timeline) const;

		System& addSystem(std::unique_ptr<System> system, TimeLine timeline);
		void removeSystem(System& system);
		Vector<System*> getSystems();
		System& getSystem(const String& name);
		Vector<std::unique_ptr<System>>& getSystems(TimeLine timeline);
		const Vector<std::unique_ptr<System>>& getSystems(TimeLine timeline) const;

		Service& addService(std::unique_ptr<Service> service);
		Service& getService(const String& name) const;

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
		HalleyAPI* api;
		std::array<Vector<std::unique_ptr<System>>, static_cast<int>(TimeLine::NUMBER_OF_TIMELINES)> systems;
		bool entityDirty = false;
		
		EntityId nextUid = 0;
		Vector<Entity*> entities;
		Vector<Entity*> entitiesPendingCreation;
		MappedPool<Entity*> entityMap;

		TreeMap<FamilyMaskType, std::unique_ptr<Family>> families;
		TreeMap<String, std::unique_ptr<Service>> services;

		mutable std::array<StopwatchAveraging, 3> timer;

		void allocateEntity(Entity* entity);
		void spawnPending();
		void updateEntities();
		void deleteEntity(Entity* entity);

		void updateSystems(TimeLine timeline, Time elapsed);
		void renderSystems(Painter& painter) const;
		
		void onAddFamily(Family& family);
	};
}
