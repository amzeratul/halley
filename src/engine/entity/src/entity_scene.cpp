#include "entity_scene.h"

#include "entity_factory.h"
#include "world.h"
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
	std::vector<const Prefab*> allowedPrefabs;
	for (auto& entry: prefabObservers) {
		if (entry.needsUpdate()) {
			allowedPrefabs.emplace_back(entry.getPrefab().get());
			entry.markUpdated();		
		}
	}

	// Update scenes
	for (auto& entry: sceneObservers) {
		entry.update(factory, entry.needsUpdate() ? PrefabObserver::UpdateMode::AllEntries : PrefabObserver::UpdateMode::AllowedPrefabs, allowedPrefabs);
		entry.markUpdated();
	}
}

void EntityScene::updateOnEditor(EntityFactory& factory)
{
	for (auto& entry: prefabObservers) {
		if (entry.needsUpdate()) {
			entry.update(factory, PrefabObserver::UpdateMode::AllEntries, {});
			entry.markUpdated();
		}
	}
	
	for (auto& entry: sceneObservers) {
		entry.markUpdated();
	}
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

void EntityScene::PrefabObserver::update(EntityFactory& factory, UpdateMode mode, const std::vector<const Prefab*>& allowedPrefabs)
{
	const auto& simpleDeltas = prefab->getSimpleDeltas();
	const auto& dataMap = prefab->getEntityDataMap();
	
	for (auto& entity: getEntities(factory.getWorld())) {
		auto deltaIter = simpleDeltas.find(entity.getInstanceUUID());
		if (deltaIter != simpleDeltas.end()) {
			// A simple delta is available for this entity, apply that
			factory.updateEntity(entity, deltaIter->second);
		} else {
			auto dataIter = dataMap.find(entity.getInstanceUUID());
			if (dataIter != dataMap.end()) {
				// Do a full update
				factory.updateEntity(entity, *dataIter->second);
			} else {
				// Not found
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
