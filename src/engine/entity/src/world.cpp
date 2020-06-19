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
#include "halley/core/api/halley_api.h"
#include "halley/support/logger.h"

using namespace Halley;

World::World(const HalleyAPI& api, Resources& resources, bool collectMetrics, CreateComponentFunction createComponent)
	: api(api)
	, resources(resources)
	, createComponent(std::move(createComponent))
	, collectMetrics(collectMetrics)
	, maskStorage(FamilyMask::MaskStorageInterface::createStorage())
	, componentDeleterTable(std::make_shared<ComponentDeleterTable>())
{
}

World::~World()
{
	for (auto& tl: systems) {
		for (auto& s: tl) {
			s->deInit();
		}
	}
	
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
	system->api = &api;
	system->resources = &resources;
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
	if (services.find(service->getName()) != services.end()) {
		throw Exception("Service already registered: " + service->getName(), HalleyExceptions::Entity);
	}
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

Service* World::tryGetService(const String& name) const
{
	const auto iter = services.find(name);
	if (iter == services.end()) {
		return nullptr;
	}
	return iter->second.get();
}

EntityRef World::createEntity(String name, std::optional<EntityRef> parent)
{
	return createEntity(UUID(), name, parent);
}

EntityRef World::createEntity(String name, EntityId parentId)
{
	return createEntity(UUID(), name, getEntity(parentId));
}

EntityRef World::createEntity(UUID uuid, String name, std::optional<EntityRef> parent)
{
	if (!uuid.isValid()) {
		uuid = UUID::generate();
	}
	
	Entity* entity = new(PoolAllocator<Entity>::alloc()) Entity();
	if (entity == nullptr) {
		throw Exception("Error creating entity - out of memory?", HalleyExceptions::Entity);
	}
	entity->uuid = uuid;
	
	entitiesPendingCreation.push_back(entity);
	allocateEntity(entity);

	auto e = EntityRef(*entity, *this);
	e.setName(std::move(name));

	if (parent) {
		e.setParent(parent.value());
	}
	
	return e;
}

EntityRef World::createEntity(UUID uuid, String name, EntityId parentId)
{
	return createEntity(uuid, name, getEntity(parentId));
}

void World::destroyEntity(EntityId id)
{
	doDestroyEntity(id);
}

void World::destroyEntity(EntityRef entity)
{
	Expects(entity.world == this);
	doDestroyEntity(entity.entity);
}

void World::doDestroyEntity(EntityId id)
{
	const auto e = tryGetRawEntity(id);
	if (e) {
		doDestroyEntity(e);
	}
}

void World::doDestroyEntity(Entity* e)
{
	e->destroy();
	entityDirty = true;
}

EntityRef World::getEntity(EntityId id)
{
	Entity* entity = tryGetRawEntity(id);
	if (entity == nullptr) {
		throw Exception("Entity does not exist: " + toString(id), HalleyExceptions::Entity);
	}
	return EntityRef(*entity, *this);
}

Entity* World::tryGetRawEntity(EntityId id)
{
	auto v = entityMap.get(id.value);
	if (v == nullptr) {
		return nullptr;
	}
	return *v;
}

std::optional<EntityRef> World::findEntity(const UUID& id)
{
	for (auto& e: entities) {
		if (e->getUUID() == id) {
			return EntityRef(*e, *this);
		}
	}
	return std::optional<EntityRef>();
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
		result.emplace_back(*e, *this);
	}
	return result;
}

std::vector<ConstEntityRef> World::getEntities() const
{
	std::vector<ConstEntityRef> result;
	result.reserve(entities.size());
	for (auto& e : entities) {
		result.emplace_back(*e, *this);
	}
	return result;
}

std::vector<EntityRef> World::getTopLevelEntities()
{
	std::vector<EntityRef> result;
	result.reserve(entities.size());
	for (auto& e : entities) {
		if (e->getParent() == nullptr) {
			result.emplace_back(*e, *this);
		}
	}
	return result;
}

std::vector<ConstEntityRef> World::getTopLevelEntities() const
{
	std::vector<ConstEntityRef> result;
	result.reserve(entities.size());
	for (auto& e : entities) {
		if (e->getParent() == nullptr) {
			result.emplace_back(*e, *this);
		}
	}
	return result;
}

void World::onEntityDirty()
{
	entityDirty = true;
}

const CreateComponentFunction& World::getCreateComponentFunction() const
{
	return createComponent;
}

MaskStorage& World::getMaskStorage() const noexcept
{
	return *maskStorage;
}

ComponentDeleterTable& World::getComponentDeleterTable()
{
	return *componentDeleterTable;
}

size_t World::sendSystemMessage(SystemMessageContext origContext, const String& targetSystem)
{
	auto& context = pendingSystemMessages.emplace_back(std::move(origContext));
	
	size_t count = 0;
	for (auto& timeline: systems) {
		for (auto& system: timeline) {
			if (system->canHandleSystemMessage(context.msgId, targetSystem)) {
				system->receiveSystemMessage(context);
				++count;
			}
		}
	}
	
	return count;
}

void World::deleteEntity(Entity* entity)
{
	Expects (entity);
	entity->destroyComponents(*componentDeleterTable);
	entity->~Entity();
	PoolAllocator<Entity>::free(entity);
}

bool World::hasSystemsOnTimeLine(TimeLine timeline) const
{
	return !getSystems(timeline).empty();
}

int64_t World::getAverageTime(TimeLine timeline) const
{
	return timer[static_cast<int>(timeline)].averageElapsedNanoSeconds();
}

void World::step(TimeLine timeline, Time elapsed)
{
	auto& t = timer[static_cast<int>(timeline)];
	if (collectMetrics) {
		t.beginSample();
	}

	spawnPending();

	initSystems();
	updateSystems(timeline, elapsed);
	processSystemMessages(timeline);

	if (collectMetrics) {
		t.endSample();
	}
}

void World::render(RenderContext& rc) const
{
	auto& t = timer[static_cast<int>(TimeLine::Render)];
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
	entity->entityId.value = res.second;
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

	entityDirty = false;

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
				entity.refresh(*maskStorage, *componentDeleterTable);
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
				if (!newMask.contains(famMask, *maskStorage)) {
					fam->removeEntity(*e.second);
				}
			}
			for (auto& e: todo.second.toAdd) {
				// Only add if the entity was not already in this
				const auto& oldMask = e.first;
				if (!oldMask.contains(famMask, *maskStorage)) {
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

void World::updateSystems(TimeLine timeline, Time elapsed)
{
	for (auto& system : getSystems(timeline)) {
		system->doUpdate(elapsed);
		spawnPending();
	}
}

void World::renderSystems(RenderContext& rc) const
{
	for (auto& system : getSystems(TimeLine::Render)) {
		system->doRender(rc);
	}
}

Family& World::addFamily(std::unique_ptr<Family> family) noexcept
{
	onAddFamily(*family);
	return *families.emplace_back(std::move(family));
}

void World::onAddFamily(Family& family) noexcept
{
	// Add any existing entities to this new family
	size_t nEntities = entities.size();
	for (size_t i = 0; i < nEntities; i++) {
		auto& entity = *entities[i];
		auto eMask = entity.getMask();
		auto fMask = family.inclusionMask;
		if ((eMask.intersection(fMask, *maskStorage)) == fMask) {
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
			if (mask.contains(famMask, *maskStorage)) {
				result.push_back(&family);
			}
		}
		familyCache[mask] = std::move(result);
		return familyCache[mask];
	}
}

void World::processSystemMessages(TimeLine timeline)
{
	bool keepRunning = true;
	while (keepRunning) {
		keepRunning = false;
		for (auto& system: systems[static_cast<int>(timeline)]) {
			system->prepareSystemMessages();
		}
		for (auto& system: systems[static_cast<int>(timeline)]) {
			system->processSystemMessages();
			if (system->getSystemMessagesInInbox() > 0) {
				keepRunning = true;
			}
		}
	}
	pendingSystemMessages.clear();
}
