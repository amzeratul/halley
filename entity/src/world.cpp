#include <iostream>
#include <chrono>
#include "world.h"
#include "system.h"
#include "family.h"

using namespace Halley;

Halley::World::World() = default;

Halley::World::~World() = default;

Halley::System& Halley::World::addSystem(std::unique_ptr<System> system)
{
	auto& ref = *system.get();
	systems.emplace_back(std::move(system));
	ref.onAddedToWorld(*this);
	systemsDirty = true;
	return ref;
}

System& Halley::World::addSystemName(String name)
{
	// TODO
	System* system = nullptr; //registry->createSystem(name);
	doAddSystem(system, name);
	return *system;
}

void World::doAddSystem(System* system, String name)
{
	systems.push_back(std::unique_ptr<System>(system));
	system->name = name;
	systemsDirty = true;
}

void World::removeSystem(System& system)
{
	for (size_t i = 0; i < systems.size(); i++) {
		if (systems[i].get() == &system) {
			systems.erase(systems.begin() + i);
			break;
		}
	}
	systemsDirty = true;
}

std::vector<System*> World::getSystems()
{
	std::vector<System*> result(systems.size());
	int i = 0;
	for (auto& s : systems) {
		result[i++] = s.get();
	}
	return std::move(result);
}

System& World::getSystem(String name)
{
	for (auto& s : systems) {
		if (s->name == name) {
			return *s.get();
		}
	}
	throw new Exception("System not found: " + name);
}

Entity& World::createEntity()
{
	Entity* entity = new(PoolAllocator<Entity>::alloc()) Entity();
	if (entity == nullptr) {
		throw Exception("Error creating entity - out of memory?");
	}
	entitiesPendingCreation.push_back(entity);
	return *entity;
}

void World::destroyEntity(EntityId id)
{
	getEntity(id).destroy();
}

Entity& World::getEntity(EntityId id)
{
	Entity* entity = tryGetEntity(id);
	if (entity == nullptr) {
		throw Exception("Entity does not exist: " + String::integerToString(id));
	}
	return *entity;
}

Entity* World::tryGetEntity(EntityId id)
{
	auto v = entityMap.get(id);
	if (v == nullptr) {
		return nullptr;
	}
	return *v;
}

size_t World::numEntities() const
{
	return entities.size();
}

void World::deleteEntity(Entity* entity)
{
	entity->~Entity();
	PoolAllocator<Entity>::free(entity);
}

void Halley::World::step(Time elapsed)
{
	using namespace std::chrono;
	auto start = high_resolution_clock::now();

	spawnPending();
	updateEntities();
	updateSystems(elapsed);

	auto end = high_resolution_clock::now();
	auto length = duration_cast<duration<double, std::milli>>(end - start).count();
	std::cout << "Step took " << length << " milliseconds." << std::endl;
}

void World::spawnPending()
{
	if (!entitiesPendingCreation.empty()) {
		for (size_t i = 0; i < entitiesPendingCreation.size(); i++) {
			auto entity = entitiesPendingCreation[i];
			auto res = entityMap.alloc();
			*res.first = entity;
			entity->uid = res.second;
		}
		std::move(entitiesPendingCreation.begin(), entitiesPendingCreation.end(), std::insert_iterator<decltype(entities)>(entities, entities.end()));
		entitiesPendingCreation.clear();
	}
}

void World::updateEntities()
{
	size_t nEntities = entities.size();

	// Update all entities
	// This loop should be as fast as reasonably possible
	for (size_t i = 0; i < nEntities; i++) {
		auto& entity = *entities[i];
		if (i + 20 < nEntities) { // Watch out for sign! Don't subtract!
			prefetchL2(entities[i + 20]);
		}

		// Check if it needs any sort of updating
		if (entity.needsRefresh()) {
			// First of all, let's check if it's dead
			if (!entity.isAlive()) {
				// Remove from systems
				for (auto& iter : families) {
					auto& family = *iter.second;
					FamilyMask::Type famMask = family.getInclusionMask();
					if ((famMask & entity.getMask()) == famMask) {
						family.removeEntity(entity);
					}
				}

				// Remove from map
				//std::cout << "-" << entity.getUID() << " ";
				entityMap.freeId(entity.getUID());

				// Swap with last, then pop
				std::swap(entities[i], entities[nEntities - 1]);
				deleteEntity(entities.back());
				entities.pop_back();
				nEntities--;
				i--;
			}
			else {
				// It's alive, so check old and new system inclusions
				FamilyMask::Type oldMask = entity.getMask();
				entity.refresh();
				FamilyMask::Type newMask = entity.getMask();

				// Did it change?
				if (oldMask != newMask) {
					// Let the systems know about it
					for (auto& iter : families) {
						auto& family = *iter.second;
						FamilyMask::Type famMask = family.getInclusionMask();
						bool matchOld = (famMask & oldMask) == famMask;
						bool matchNew = (famMask & newMask) == famMask;

						// Remove
						if (matchOld && !matchNew) {
							family.removeEntity(entity);
						}

						// Add
						if (!matchOld && matchNew) {
							family.addEntity(entity);
						}
					}
				}
			}
		}
	}
}

void World::updateSystems(Time)
{
	// Update families
	for (auto& iter : families) {
		auto& family = iter.second;
		family->removeDeadEntities();
	}

	// Update systems
	for (auto& system : systems) {
		system->step();
	}
}

void World::onAddFamily(Family& family)
{
	// Add any existing entities to this new family
	size_t nEntities = entities.size();
	for (size_t i = 0; i < nEntities; i++) {
		auto& entity = *entities[i];
		auto eMask = entity.getMask();
		auto fMask = family.getInclusionMask();
		if ((eMask & fMask) == fMask) {
			family.addEntity(entity);
		}
	}
}
