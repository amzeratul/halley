#include <iostream>
#include <chrono>
#include <halley/support/exception.h>
#include <halley/data_structures/memory_pool.h>
#include <halley/utils/utils.h>
#include "world.h"
#include "system.h"
#include "family.h"
#include "halley/text/string_converter.h"

using namespace Halley;

Halley::World::World(HalleyAPI* api)
	: api(api)
{	
}

Halley::World::~World()
{
	for (auto& f: families) {
		//f.second->clearEntities();
		f->clearEntities();
	}
	for (auto& tl: systems) {
		tl.clear();
	}
	for (auto e: entitiesPendingCreation) {
		deleteEntity(e);
	}
	for (auto e: entities) {
		deleteEntity(e);
	}
	families.clear();
	services.clear();
}

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
	return result;
}

System& World::getSystem(const String& name)
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

Service& World::addService(std::shared_ptr<Service> service)
{
	auto& ref = *service;
	services[service->getName()] = std::move(service);
	return ref;
}

Service& World::getService(const String& name) const
{
	auto iter = services.find(name);
	if (iter == services.end()) {
		throw Exception("Service not found: " + name);
	}
	return *iter->second;
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
	auto e = tryGetEntity(id);
	if (e) {
		e->destroy();
		entityDirty = true;
	}
}

EntityRef World::getEntity(EntityId id)
{
	Entity* entity = tryGetEntity(id);
	if (entity == nullptr) {
		throw Exception("Entity does not exist: " + toString(id));
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

	initSystems();
	updateSystems(timeline, elapsed);
	
	if (timeline == TimeLine::VariableUpdate) {
		// The variable update timeline runs before render, so give everything a chance to spawn before rendering
		spawnPending();
		updateEntities();
	}

	t.endSample();
}

void World::render(RenderContext& rc) const
{
	auto& t = timer[int(TimeLine::Render)];
	t.beginSample();

	initSystems();
	renderSystems(rc);

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

	std::vector<size_t> entitiesRemoved;

	struct FamilyTodo {
		std::vector<Entity*> toAdd;
		std::vector<Entity*> toRemove;
	};
	std::map<FamilyMaskType, FamilyTodo> pending;

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
				pending[entity.getMask()].toRemove.push_back(&entity);
				entitiesRemoved.push_back(i);
			} else {
				// It's alive, so check old and new system inclusions
				FamilyMaskType oldMask = entity.getMask();
				entity.refresh();
				FamilyMaskType newMask = entity.getMask();

				// Did it change?
				if (oldMask != newMask) {
					pending[oldMask].toRemove.push_back(&entity);
					pending[newMask].toAdd.push_back(&entity);
				}
			}
		}
	}

	for (auto& todo: pending) {
		for (auto& fam: getFamiliesFor(todo.first)) {
			for (auto& e: todo.second.toRemove) {
				fam->removeEntity(*e);
			}
			for (auto& e: todo.second.toAdd) {
				fam->addEntity(*e);
			}
		}
	}

	// Update families
	for (auto& iter : families) {
		//auto& family = iter.second;
		auto& family = iter;
		family->updateEntities();
	}

	// Actually remove dead entities
	if (!entitiesRemoved.empty()) {
		size_t livingEntityCount = entities.size();
		for (int i = int(entitiesRemoved.size()); --i >= 0; ) {
			size_t idx = entitiesRemoved[i];
			auto& entity = *entities[idx];

			// Remove
			entityMap.freeId(entity.getEntityId());
			deleteEntity(&entity);

			// Put it at the back of the array, so it's removed when the array gets resized
			std::swap(entities[idx], entities[--livingEntityCount]);
		}
		entities.resize(livingEntityCount);
	}

	entityDirty = false;
}

void World::initSystems() const
{
	for (auto& tl: systems) {
		for (auto& system : tl) {
			system->tryInit();
		}
	}
}

void World::updateSystems(TimeLine timeline, Time time)
{
	for (auto& system : getSystems(timeline)) {
		system->doUpdate(time);
	}
}

void World::renderSystems(RenderContext& rc) const
{
	for (auto& system : getSystems(TimeLine::Render)) {
		system->doRender(rc);
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
	familyCache.clear();
}

const std::vector<Family*>& World::getFamiliesFor(const FamilyMaskType& mask)
{
	auto i = familyCache.find(mask);
	if (i != familyCache.end()) {
		return i->second;
	} else {
		std::vector<Family*> result;
		for (auto& iter : families) {
			auto& family = *iter;
			FamilyMaskType famMask = family.inclusionMask;
			if (mask.contains(famMask)) {
				result.push_back(&family);
			}
		}
		familyCache[mask] = std::move(result);
		return familyCache[mask];
	}
}
