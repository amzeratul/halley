#include <iostream>
#include <chrono>
#include <halley/support/exception.h>
#include <halley/data_structures/memory_pool.h>
#include <halley/utils/utils.h>
#include "world.h"
#include "system.h"
#include "family.h"

using namespace Halley;

Halley::World::World(HalleyAPI* api)
	: api(api)
{	
}

Halley::World::~World() = default;

Halley::System& Halley::World::addSystem(std::unique_ptr<System> system, TimeLine timelineType)
{
	system->api = api;
	auto& ref = *system.get();
	auto& timeline = getSystems(timelineType);
	timeline.emplace_back(std::move(system));
	ref.onAddedToWorld(*this, int(timeline.size()));
	return ref;
}

void World::removeSystem(System& system)
{
	for (auto& sys : systems) {
		for (size_t i = 0; i < sys.size(); i++) {
			if (sys[i].get() == &system) {
				sys.erase(sys.begin() + i);
				return;
			}
		}
	}
}

Vector<std::unique_ptr<System>>& World::getSystems(TimeLine timeline)
{
	return systems[static_cast<int>(timeline)];
}

const Vector<std::unique_ptr<System>>& World::getSystems(TimeLine timeline) const
{
	return systems[static_cast<int>(timeline)];
}

Vector<System*> World::getSystems()
{
	size_t n = 0;
	for (auto& tl : systems) {
		n += tl.size();
	}
	Vector<System*> result(n);

	int i = 0;
	for (auto& tl : systems) {
		for (auto& s : tl) {
			result[i++] = s.get();
		}
	}
	return std::move(result);
}

System& World::getSystem(String name)
{
	for (auto& tl : systems) {
		for (auto& s : tl) {
			if (s->name == name) {
				return *s.get();
			}
		}
	}
	throw Exception("System not found: " + name);
}

EntityRef World::createEntity()
{
	Entity* entity = new(PoolAllocator<Entity>::alloc()) Entity();
	if (entity == nullptr) {
		throw Exception("Error creating entity - out of memory?");
	}
	entitiesPendingCreation.push_back(entity);
	allocateEntity(entity);
	return EntityRef(*entity, *this);
}

void World::destroyEntity(EntityId id)
{
	getEntity(id).entity.destroy();
	entityDirty = true;
}

EntityRef World::getEntity(EntityId id)
{
	Entity* entity = tryGetEntity(id);
	if (entity == nullptr) {
		throw Exception("Entity does not exist: " + String::integerToString(id));
	}
	return EntityRef(*entity, *this);
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

void World::onEntityDirty()
{
	entityDirty = true;
}

void World::deleteEntity(Entity* entity)
{
	entity->~Entity();
	PoolAllocator<Entity>::free(entity);
}

bool World::hasSystemsOnTimeLine(TimeLine timeline) const
{
	return getSystems(timeline).size() > 0;
}

long long World::getAverageTime(TimeLine timeline) const
{
	return timer[int(timeline)].averageElapsedNanoSeconds();
}

void Halley::World::step(TimeLine timeline, Time elapsed)
{
	auto& t = timer[int(timeline)];
	t.beginSample();

	spawnPending();
	updateEntities();
	updateSystems(timeline, elapsed);

	t.endSample();
}

void World::render(Painter& painter) const
{
	auto& t = timer[int(TimeLine::Render)];
	t.beginSample();

	renderSystems(painter);

	t.endSample();
}

void World::allocateEntity(Entity* entity) {
	auto res = entityMap.alloc();
	*res.first = entity;
	entity->uid = res.second;
}

void World::spawnPending()
{
	if (!entitiesPendingCreation.empty()) {
		for (auto& e : entitiesPendingCreation) {
			e->onReady();
		}
		std::move(entitiesPendingCreation.begin(), entitiesPendingCreation.end(), std::insert_iterator<decltype(entities)>(entities, entities.end()));
		entitiesPendingCreation.clear();
		entityDirty = true;
	}
}

void World::updateEntities()
{
	if (!entityDirty) {
		return;
	}

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
					FamilyMaskType famMask = family.inclusionMask;
					if ((famMask & entity.getMask()) == famMask) {
						family.removeEntity(entity);
					}
				}

				// Remove from map
				//std::cout << "-" << entity.getEntityId() << " ";
				entityMap.freeId(entity.getEntityId());

				// Swap with last, then pop
				std::swap(entities[i], entities[nEntities - 1]);
				deleteEntity(entities.back());
				entities.pop_back();
				nEntities--;
				i--;
			}
			else {
				// It's alive, so check old and new system inclusions
				FamilyMaskType oldMask = entity.getMask();
				entity.refresh();
				FamilyMaskType newMask = entity.getMask();

				// Did it change?
				if (oldMask != newMask) {
					// Let the systems know about it
					for (auto& iter : families) {
						auto& family = *iter.second;
						FamilyMaskType famMask = family.inclusionMask;
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

	// Update families
	for (auto& iter : families) {
		auto& family = iter.second;
		family->removeDeadEntities();
	}

	entityDirty = false;
}

void World::updateSystems(TimeLine timeline, Time time)
{
	// Update systems
	for (auto& system : getSystems(timeline)) {
		system->doUpdate(time);
	}
}

void World::renderSystems(Painter& painter) const
{
	// Update systems
	for (auto& system : getSystems(TimeLine::Render)) {
		system->doRender(painter);
	}
}

void World::onAddFamily(Family& family)
{
	// Add any existing entities to this new family
	size_t nEntities = entities.size();
	for (size_t i = 0; i < nEntities; i++) {
		auto& entity = *entities[i];
		auto eMask = entity.getMask();
		auto fMask = family.inclusionMask;
		if ((eMask & fMask) == fMask) {
			family.addEntity(entity);
		}
	}
}
