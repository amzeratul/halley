#include "entity_scene.h"

#include "entity_factory.h"
#include "world.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
using namespace Halley;

std::vector<EntityRef>& EntityScene::getEntities()
{
	return entities;
}

const std::vector<EntityRef>& EntityScene::getEntities() const
{
	return entities;
}

bool EntityScene::needsUpdate() const
{
	for (const auto& entry: sceneObservers) {
		if (entry.needsUpdate()) {
			return true;
		}
	}
	for (const auto& entry: prefabObservers) {
		if (entry.needsUpdate()) {
			return true;
		}
	}
	return false;
}

void EntityScene::update(EntityFactory& factory)
{
	// Collect all prefabs that changed
	for (auto& entry: prefabObservers) {
		if (entry.needsUpdate()) {
			entry.updateEntities(factory, PrefabObserver::UpdateMode::DeltaOnly);
			entry.markUpdated();		
		}
	}

	// Update scenes
	for (auto& entry: sceneObservers) {
		if (entry.needsUpdate()) {
			entry.updateEntities(factory, PrefabObserver::UpdateMode::AllEntries);
			entry.markUpdated();
		}
	}
}

void EntityScene::updateOnEditor(EntityFactory& factory)
{
	update(factory);
}

void EntityScene::addPrefabReference(const std::shared_ptr<const Prefab>& prefab, const EntityRef& entity, std::optional<int> index)
{
	getOrMakeObserver(prefab).addEntity(entity, index);
}

void EntityScene::addRootEntity(EntityRef entity)
{
	entities.emplace_back(entity);
}

EntityScene::PrefabObserver::PrefabObserver(std::shared_ptr<const Prefab> prefab)
	: prefab(std::move(prefab))
{
	assetVersion = this->prefab->getAssetVersion();
}

bool EntityScene::PrefabObserver::needsUpdate() const
{
	return assetVersion != prefab->getAssetVersion();
}

void EntityScene::PrefabObserver::updateEntities(EntityFactory& factory, UpdateMode mode) const
{
	const auto& modified = prefab->getEntitiesModified();
	const auto& removed = prefab->getEntitiesRemoved();
	const auto& dataMap = prefab->getEntityDataMap();

	if (!prefab->isScene()) {
		assert(modified.size() == 1 && removed.empty());
	}

	for (auto& entity: getEntities(factory.getWorld())) {
		const auto& uuid = entity.getInstanceUUID();
		
		auto deltaIter = modified.find(uuid);
		if (deltaIter != modified.end()) {
			// A simple delta is available for this entity, apply that
			factory.updateEntity(entity, deltaIter->second);
		} else if (mode == UpdateMode::AllEntries) {
			auto dataIter = dataMap.find(uuid);
			if (dataIter != dataMap.end()) {
				// Do a full update
				factory.updateEntity(entity, *dataIter->second);
			} else if (removed.find(uuid) != removed.end()) {
				// Remove
				factory.getWorld().destroyEntity(entity);
			} else {
				// Not found
				Logger::logError("PrefabObserver::update error: UUID " + uuid.toString() + " not found in prefab " + prefab->getAssetId());
			}
		}
	}
}

void EntityScene::PrefabObserver::markUpdated()
{
	assetVersion = prefab->getAssetVersion();
}

void EntityScene::PrefabObserver::addEntity(EntityRef entity, std::optional<int> index)
{
	std_ex::contains(entityIds, entity.getEntityId());
	entityIds.push_back(entity.getEntityId());
}

const std::shared_ptr<const Prefab>& EntityScene::PrefabObserver::getPrefab() const
{
	return prefab;
}

std::vector<EntityRef> EntityScene::PrefabObserver::getEntities(World& world) const
{
	std::vector<EntityRef> entities;
	for (const auto& id: entityIds) {
		auto* entity = world.tryGetRawEntity(id);
		if (entity) {
			entities.emplace_back(*entity, world);
		}
	}
	return entities;
}

EntityScene::PrefabObserver& EntityScene::getOrMakeObserver(const std::shared_ptr<const Prefab>& prefab)
{
	auto& list = prefab->isScene() ? sceneObservers : prefabObservers;
	
	for (auto& o: list) {
		if (o.getPrefab() == prefab) {
			return o;
		}
	}
	return list.emplace_back(prefab);
}
