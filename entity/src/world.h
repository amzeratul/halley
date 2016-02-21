#pragma once

#include <memory>
#include <vector>
#include "entity_id.h"

namespace Halley {
	class Entity;
	class System;

	class World
	{
	public:
		World();
		~World();

		void step();

		System& addSystem(std::unique_ptr<System> system);
		void removeSystem(System& system);
		System& getSystem(String name);

		Entity& createEntity();
		void destroyEntity(EntityId id);
		Entity& getEntity(EntityId id);
		Entity* tryGetEntity(EntityId id);
		size_t numEntities() const;
		
	private:
		std::vector<std::unique_ptr<System>> systems;
	};
}
