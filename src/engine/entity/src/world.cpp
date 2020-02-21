#include <iostream>
#include <chrono>
#include <halley/support/exception.h>
#include <halley/data_structures/memory_pool.h>
#include <halley/utils/utils.h>
#include "world.h"
#include "system.h"
#include "family.h"
#include "halley/text/string_converter.h"
#include "halley/support/debug.h"
#include "halley/file_formats/config_file.h"
#include "halley/maths/uuid.h"

using namespace Halley;

World::World(const HalleyAPI* api, bool collectMetrics, CreateComponentFunction createComponent)
	: api(api)
	, createComponent(std::move(createComponent))
	, collectMetrics(collectMetrics)
{	
}

World::~World()
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

System& World::addSystem(std::unique_ptr<System> system, TimeLine timelineType)
{
	system->api = api;
	system->setCollectSamples(collectMetrics);
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
	throw Exception("System not found: " + name, HalleyExceptions::Entity);
}

Service& World::addService(std::shared_ptr<Service> service)
{
	auto& ref = *service;
	services[service->getName()] = std::move(service);
	return ref;
}

void World::loadSystems(const ConfigNode& root, std::function<std::unique_ptr<System>(String)> createFunction)
{
	auto timelines = root["timelines"].asMap();
	for (auto iter = timelines.begin(); iter != timelines.end(); ++iter) {
		String timelineName = iter->first;
		TimeLine timeline;
		if (timelineName == "fixedUpdate") {
			timeline = TimeLine::FixedUpdate;
		} else if (timelineName == "variableUpdate") {
			timeline = TimeLine::VariableUpdate;
		} else if (timelineName == "render") {
			timeline = TimeLine::Render;
		} else {
			throw Exception("Unknown timeline: " + timelineName, HalleyExceptions::Entity);
		}

		for (auto& sysName: iter->second) {
			String name = sysName.asString();
			addSystem(createFunction(name + "System"), timeline).setName(name);
		}
	}
}

Service& World::getService(const String& name) const
{
	auto iter = services.find(name);
	if (iter == services.end()) {
		throw Exception("Service not found: " + name, HalleyExceptions::Entity);
	}
	return *iter->second;
}

EntityRef World::createEntity(UUID uuid, String name)
{
	Entity* entity = new(PoolAllocator<Entity>::alloc()) Entity();
	if (entity == nullptr) {
		throw Exception("Error creating entity - out of memory?", HalleyExceptions::Entity);
	}
	entity->uuid = uuid;
	entitiesPendingCreation.push_back(entity);
	allocateEntity(entity);
	auto e = EntityRef(*entity, *this);
	e.setName(std::move(name));
	return e;
}

EntityRef World::createEntity(String name)
{
	return createEntity(UUID::generate(), name);
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
		throw Exception("Entity does not exist: " + toString(id), HalleyExceptions::Entity);
	}
	return EntityRef(*entity, *this);
}

Entity* World::tryGetEntity(EntityId id)
{
	auto v = entityMap.get(id.value);
	if (v == nullptr) {
		return nullptr;
	}
	return *v;
}

size_t World::numEntities() const
{
	return entities.size();
}

std::vector<EntityRef> World::getEntities()
{
	std::vector<EntityRef> result;
	result.reserve(entities.size());
	for (auto& e: entities) {
		result.push_back(EntityRef(*e, *this));
	}
	return result;
}

void World::onEntityDirty()
{
	entityDirty = true;
}

const World::CreateComponentFunction& World::getCreateComponentFunction() const
{
	return createComponent;
}

void World::deleteEntity(Entity* entity)
{
	Expects (entity);
	entity->~Entity();
	PoolAllocator<Entity>::free(entity);
}

bool World::hasSystemsOnTimeLine(TimeLine timeline) const
{
	return getSystems(timeline).size() > 0;
}

int64_t World::getAverageTime(TimeLine timeline) const
{
	return timer[int(timeline)].averageElapsedNanoSeconds();
}

void World::step(TimeLine timeline, Time elapsed)
{
	auto& t = timer[int(timeline)];
	if (collectMetrics) {
		t.beginSample();
	}

	spawnPending();

	initSystems();
	updateSystems(timeline, elapsed);

	if (collectMetrics) {
		t.endSample();
	}
}

void World::render(RenderContext& rc) const
{
	auto& t = timer[int(TimeLine::Render)];
	if (collectMetrics) {
		t.beginSample();
	}

	renderSystems(rc);

	if (collectMetrics) {
		t.endSample();
	}
}

void World::allocateEntity(Entity* entity) {
	auto res = entityMap.alloc();
	*res.first = entity;
	entity->uid.value = res.second;
}

void World::spawnPending()
{
	if (!entitiesPendingCreation.empty()) {
		HALLEY_DEBUG_TRACE();
		for (auto& e : entitiesPendingCreation) {
			e->onReady();
		}
		std::move(entitiesPendingCreation.begin(), entitiesPendingCreation.end(), std::insert_iterator<decltype(entities)>(entities, entities.end()));
		entitiesPendingCreation.clear();
		entityDirty = true;
		HALLEY_DEBUG_TRACE();
	}

	updateEntities();
}

void World::updateEntities()
{
	if (!entityDirty) {
		return;
	}

	HALLEY_DEBUG_TRACE();
	size_t nEntities = entities.size();

	std::vector<size_t> entitiesRemoved;

	struct FamilyTodo {
		std::vector<std::pair<FamilyMaskType, Entity*>> toAdd;
		std::vector<std::pair<FamilyMaskType, Entity*>> toRemove;
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
				pending[entity.getMask()].toRemove.emplace_back(FamilyMaskType(), &entity);
				entitiesRemoved.push_back(i);
			} else {
				// It's alive, so check old and new system inclusions
				FamilyMaskType oldMask = entity.getMask();
				entity.refresh();
				FamilyMaskType newMask = entity.getMask();

				// Did it change?
				if (oldMask != newMask) {
					pending[oldMask].toRemove.emplace_back(newMask, &entity);
					pending[newMask].toAdd.emplace_back(oldMask, &entity);
				}
			}
		}
	}

	HALLEY_DEBUG_TRACE();
	// Go through every family adding/removing entities as needed
	for (auto& todo: pending) {
		for (auto& fam: getFamiliesFor(todo.first)) {
			const auto& famMask = fam->inclusionMask;
			
			for (auto& e: todo.second.toRemove) {
				// Only remove if the entity is not about to be re-added
				const auto& newMask = e.first;
				if (!newMask.contains(famMask)) {
					fam->removeEntity(*e.second);
				}
			}
			for (auto& e: todo.second.toAdd) {
				// Only add if the entity was not already in this
				const auto& oldMask = e.first;
				if (!oldMask.contains(famMask)) {
					fam->addEntity(*e.second);
				}
			}
		}
	}

	HALLEY_DEBUG_TRACE();
	// Update families
	for (auto& iter : families) {
		iter->updateEntities();
	}

	HALLEY_DEBUG_TRACE();
	// Actually remove dead entities
	if (!entitiesRemoved.empty()) {
		size_t livingEntityCount = entities.size();
		for (int i = int(entitiesRemoved.size()); --i >= 0; ) {
			size_t idx = entitiesRemoved[i];
			auto& entity = *entities[idx];

			// Remove
			entityMap.freeId(entity.getEntityId().value);
			deleteEntity(&entity);

			// Put it at the back of the array, so it's removed when the array gets resized
			std::swap(entities[idx], entities[--livingEntityCount]);
		}
		entities.resize(livingEntityCount);
	}

	entityDirty = false;
	HALLEY_DEBUG_TRACE();
}

void World::initSystems()
{
	for (auto& tl: systems) {
		for (auto& system : tl) {
			// If the system is initialised, also check for any entities that need spawning
			if (system->tryInit()) {
				spawnPending();
			}
		}
	}
}

void World::updateSystems(TimeLine timeline, Time time)
{
	for (auto& system : getSystems(timeline)) {
		system->doUpdate(time);
		spawnPending();
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
