#include <iostream>
#include <chrono>
#include <halley/support/exception.h>
#include <halley/utils/utils.h>
#include "world.h"

#include <cassert>

#include "system.h"
#include "family.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/text/string_converter.h"
#include "halley/support/debug.h"
#include "halley/file_formats/config_file.h"
#include "halley/maths/uuid.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/render_context.h"
#include "halley/support/logger.h"
#include "halley/support/profiler.h"

using namespace Halley;

World::World(const HalleyAPI& api, Resources& resources, CreateComponentFunction createComponent, CreateMessageFunction createMessage, CreateSystemMessageFunction createSystemMessage)
	: api(api)
	, resources(resources)
	, createComponent(std::move(createComponent))
	, createMessage(std::move(createMessage))
	, createSystemMessage(std::move(createSystemMessage))
	, maskStorage(FamilyMask::MaskStorageInterface::createStorage())
	, componentDeleterTable(std::make_shared<ComponentDeleterTable>())
	, entityPool(std::make_shared<PoolAllocator<Entity>>())
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

std::unique_ptr<World> World::make(const HalleyAPI& api, Resources& resources, const String& sceneName, bool devMode)
{
	auto world = std::make_unique<World>(api, resources, CreateEntityFunctions::getCreateComponent(), CreateEntityFunctions::getCreateMessage(), CreateEntityFunctions::getCreateSystemMessage());
	const auto& sceneConfig = resources.get<ConfigFile>(sceneName)->getRoot();
	world->loadSystems(sceneConfig, CreateEntityFunctions::getCreateSystem());
	return world;
}

System& World::addSystem(std::unique_ptr<System> system, TimeLine timelineType)
{
	system->api = &api;
	system->resources = &resources;
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

EntityRef World::createEntity(UUID uuid, String name, EntityId parentId)
{
	return createEntity(uuid, name, getEntity(parentId));
}

EntityRef World::createEntity(UUID uuid, String name, std::optional<EntityRef> parent, uint8_t worldPartition)
{
	if (!uuid.isValid()) {
		uuid = UUID::generate();
	}
	
	Entity* entity = new(entityPool->alloc()) Entity();
	if (entity == nullptr) {
		throw Exception("Error creating entity - out of memory?", HalleyExceptions::Entity);
	}
	entity->instanceUUID = uuid;
	entity->worldPartition = worldPartition;

	entitiesPendingCreation.push_back(entity);
	allocateEntity(entity);

	auto e = EntityRef(*entity, *this);
	e.setName(std::move(name));

	if (parent) {
		e.setParent(parent.value());
	}

	assert(uuidMap.find(uuid) == uuidMap.end()); // No duplicate UUIDs allowed
	uuidMap[uuid] = entity;
	
	return e;
}

void World::destroyEntity(EntityId id)
{
	doDestroyEntity(id);
}

void World::destroyEntity(EntityRef entity)
{
	if (!entity.isValid()) {
		Logger::logWarning("Attempting to destroy invalid EntityRef.");
		return;
	}
	if (!entity.entity->isAlive()) {
		Logger::logWarning("Attempting to destroy entity which is already dead.");
		return;
	}
	if (entity.world != this) {
		throw Exception("Attempting to destroy an entity which does not belong to this world.", HalleyExceptions::Entity);
	}
	
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

ConstEntityRef World::getEntity(EntityId id) const
{
	const Entity* entity = tryGetRawEntity(id);
	if (entity == nullptr) {
		throw Exception("Entity does not exist: " + toString(id), HalleyExceptions::Entity);
	}
	return ConstEntityRef(*entity, *this);
}

EntityRef World::tryGetEntity(EntityId id)
{
	Entity* entity = tryGetRawEntity(id);
	return entity ? EntityRef(*entity, *this) : EntityRef();
}

ConstEntityRef World::tryGetEntity(EntityId id) const
{
	const Entity* entity = tryGetRawEntity(id);
	return entity ? ConstEntityRef(*entity, *this) : ConstEntityRef();
}

Entity* World::tryGetRawEntity(EntityId id)
{
	auto* v = entityMap.get(id.value);
	if (v == nullptr) {
		return nullptr;
	}
	return *v;
}

const Entity* World::tryGetRawEntity(EntityId id) const
{
	const auto* v = entityMap.get(id.value);
	if (v == nullptr) {
		return nullptr;
	}
	return *v;
}

std::optional<EntityRef> World::findEntity(const UUID& id, bool includePending)
{
	const auto result = uuidMap.find(id);
	if (result != uuidMap.end()) {
		Ensures(result->second->getInstanceUUID() == id);
		return EntityRef(*result->second, *this);
	}
	return {};
}

size_t World::numEntities() const
{
	return entities.size();
}

Vector<EntityRef> World::getEntities()
{
	Vector<EntityRef> result;
	result.reserve(entities.size());
	for (auto& e: entities) {
		result.emplace_back(*e, *this);
	}
	return result;
}

Vector<ConstEntityRef> World::getEntities() const
{
	Vector<ConstEntityRef> result;
	result.reserve(entities.size());
	for (auto& e : entities) {
		result.emplace_back(*e, *this);
	}
	return result;
}

Vector<EntityRef> World::getTopLevelEntities()
{
	Vector<EntityRef> result;
	result.reserve(entities.size());
	for (auto& e : entities) {
		if (e->getParent() == nullptr) {
			result.emplace_back(*e, *this);
		}
	}
	return result;
}

Vector<ConstEntityRef> World::getTopLevelEntities() const
{
	Vector<ConstEntityRef> result;
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

void World::setEntityReloaded()
{
	entityReloaded = true;
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

size_t World::sendSystemMessage(SystemMessageContext origContext, const String& targetSystem, SystemMessageDestination destination)
{
	// Choose where to send
	const bool amITheHost = !networkInterface || networkInterface->isHost();
	bool sendLocal = false;
	bool sendRemote = false;
	switch (destination) {
	case SystemMessageDestination::Local:
		sendLocal = true;
		break;
	case SystemMessageDestination::AllClients:
		sendLocal = true;
		sendRemote = true;
		break;
	case SystemMessageDestination::Host:
		sendLocal = amITheHost;
		sendRemote = !amITheHost;
		break;
	case SystemMessageDestination::RemoteClients:
		sendLocal = false;
		sendRemote = true;
		break;
	}
	
	auto& context = pendingSystemMessages.emplace_back(std::move(origContext));

	size_t systemCount = 0;
	for (auto& timeline : systems) {
		for (auto& system : timeline) {
			if (system->canHandleSystemMessage(context.msgId, targetSystem)) {
				if (sendLocal) {
					system->receiveSystemMessage(context);
				}
				++systemCount;
			}
		}
	}

	size_t totalCount = 0;
	if (sendLocal) {
		totalCount += systemCount;
	}

	if (sendRemote) {
		sendNetworkSystemMessage(targetSystem, context, destination);
		totalCount += (destination == SystemMessageDestination::Host ? 1 : 2) * systemCount; // Assume at least two clients for non-host sends
	}
		
	return totalCount;
}

bool World::isDevMode() const
{
	return api.core->isDevMode();
}

void World::setEditor(bool isEditor)
{
	editor = isEditor;
}

bool World::isEditor() const
{
	return editor;
}

void World::deleteEntity(Entity* entity)
{
	Expects (entity);
	entity->destroyComponents(*componentDeleterTable);
	entity->~Entity();
	entityPool->free(entity);
}

bool World::hasSystemsOnTimeLine(TimeLine timeline) const
{
	return !getSystems(timeline).empty();
}

void World::step(TimeLine timeline, Time elapsed)
{
	//ProfilerEvent event(timeline == TimeLine::FixedUpdate ? ProfilerEventType::WorldFixedUpdate : ProfilerEventType::WorldVariableUpdate);

	spawnPending();

	initSystems(std::array<TimeLine, 3>{ TimeLine::FixedUpdate, TimeLine::VariableUpdate, TimeLine::Render });
	updateSystems(timeline, elapsed);
	processSystemMessages(timeline);
}

void World::render(RenderContext& rc)
{
	//ProfilerEvent event(ProfilerEventType::WorldSystemRender);

	initSystems(std::array<TimeLine, 3>{ TimeLine::FixedUpdate, TimeLine::VariableUpdate, TimeLine::Render });
	renderSystems(rc);
	rc.flush();
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

	Vector<size_t> entitiesRemoved;

	struct FamilyTodo {
		Vector<std::pair<FamilyMaskType, Entity*>> toAdd;
		Vector<std::pair<FamilyMaskType, Entity*>> toRemove;
		Vector<std::pair<FamilyMaskType, Entity*>> toReload;
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

	if (entityReloaded) {
		for (size_t i = 0; i < nEntities; i++) {
			auto& entity = *entities[i];
			if (entity.reloaded && entity.isAlive()) {
				pending[entity.getMask()].toReload.emplace_back(entity.getMask(), &entity);
				entity.reloaded = false;
			}
		}
	}

	entityReloaded = false;

	HALLEY_DEBUG_TRACE();
	// Go through every family adding/removing entities as needed
	for (auto& todo: pending) {
		for (auto* fam: getFamiliesFor(todo.first)) {
			const auto& famMask = fam->inclusionMask;
			const auto& optFamMask = fam->optionalMask;
			auto& ms = *maskStorage;
			
			for (auto& e: todo.second.toRemove) {
				// Only remove if the entity is not about to be re-added
				const auto& newMask = e.first;
				if (!newMask.contains(famMask, ms)) {
					fam->removeEntity(*e.second);
				}
			}
			for (auto& e: todo.second.toAdd) {
				// Only add if the entity was not already in this
				const auto& oldMask = e.first;
				const auto& newMask = todo.first;
				if (!oldMask.contains(famMask, ms)) {
					fam->addEntity(*e.second);
				} else if (optFamMask.unionChangedBetween(oldMask, newMask, ms)) {
					// Needs refreshing of optional references
					fam->refreshEntity(*e.second);
				}
			}

			for (auto& e : todo.second.toReload) {
				fam->reloadEntity(*e.second);
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
			uuidMap.erase(entity.getInstanceUUID());
			deleteEntity(&entity);

			// Put it at the back of the array, so it's removed when the array gets resized
			std::swap(entities[idx], entities[--livingEntityCount]);
		}
		entities.resize(livingEntityCount);
	}

	HALLEY_DEBUG_TRACE();
}

void World::initSystems(gsl::span<const TimeLine> timelines)
{
	for (auto& tl: timelines) {
		for (auto& system : systems[int(tl)]) {
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

const Vector<Family*>& World::getFamiliesFor(const FamilyMaskType& mask)
{
	auto i = familyCache.find(mask);
	if (i != familyCache.end()) {
		return i->second;
	} else {
		Vector<Family*> result;
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
	auto& timelineSystems = systems[static_cast<int>(timeline)];
	while (keepRunning) {
		spawnPending();
		
		keepRunning = false;
		for (auto& system: timelineSystems) {
			system->prepareSystemMessages();
		}
		for (auto& system: timelineSystems) {
			system->processSystemMessages();
		}
		for (auto& system : timelineSystems) {
			if (system->getSystemMessagesInInbox() > 0) {
				keepRunning = true;
			}
		}
	}
	pendingSystemMessages.clear();
}

bool World::isEntityNetworkRemote(EntityId entityId) const
{
	return isEntityNetworkRemote(getEntity(entityId));
}

bool World::isEntityNetworkRemote(EntityRef entity) const
{
	if (networkInterface) {
		return networkInterface->isRemote(entity);
	}
	return false;
}

bool World::isEntityNetworkRemote(ConstEntityRef entity) const
{
	if (networkInterface) {
		return networkInterface->isRemote(entity);
	}
	return false;
}

void World::sendNetworkMessage(EntityId entityId, int messageId, std::unique_ptr<Message> msg)
{
	if (networkInterface) {
		auto options = SerializerOptions(SerializerOptions::maxVersion);
		options.world = this;
		networkInterface->sendEntityMessage(getEntity(entityId), messageId, Serializer::toBytes(*msg, options));
	}
}

void World::sendNetworkSystemMessage(const String& targetSystem, const SystemMessageContext& context, SystemMessageDestination destination)
{
	if (networkInterface) {
		auto options = SerializerOptions(SerializerOptions::maxVersion);
		options.world = this;
		networkInterface->sendSystemMessage(targetSystem, context.msgId, Serializer::toBytes(*context.msg, options), destination, context.callback);
	}
}

std::unique_ptr<Message> World::deserializeMessage(int msgId, gsl::span<const std::byte> data)
{
	auto msg = createMessage(msgId);

	auto options = SerializerOptions(SerializerOptions::maxVersion);
	options.world = this;
	Deserializer::fromBytes(*msg, data, options);

	return msg;
}

std::unique_ptr<SystemMessage> World::deserializeSystemMessage(int msgId, gsl::span<const std::byte> data)
{
	auto msg = createSystemMessage(msgId);

	auto options = SerializerOptions(SerializerOptions::maxVersion);
	options.world = this;
	Deserializer::fromBytes(*msg, data, options);

	return msg;
}

void World::setNetworkInterface(IWorldNetworkInterface* interface)
{
	networkInterface = interface;
}
