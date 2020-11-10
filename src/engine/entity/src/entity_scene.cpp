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
	for (auto& entry: prefabObservers) {
		if (entry.needsUpdate()) {
			return true;
		}
	}
	return false;
}

void EntityScene::update(EntityFactory& factory)
{
	// When an update is requested, only actually update the scene, regardless of whichever triggered it
	// However, mark all of them as updated.
	// 
	for (auto& entry: prefabObservers) {
		if (entry.isScene()) {
			entry.update(factory);
		}
		entry.markUpdated();
	}
}

void EntityScene::updateOnEditor(EntityFactory& factory)
{
	for (auto& entry : prefabObservers) {
		if (entry.needsUpdate()) {
			if (!entry.isScene()) {
				entry.update(factory);
			}
			entry.markUpdated();
		}
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

bool EntityScene::PrefabObserver::isScene() const
{
	return scene;
}

void EntityScene::PrefabObserver::update(EntityFactory& factory)
{
	auto& world = factory.getWorld();
	std::vector<EntityRef> entities;
	for (const auto& id: entityIds) {
		auto* entity = world.tryGetRawEntity(id);
		if (entity) {
			entities.emplace_back(*entity, world);
		}
	}

	if (!entities.empty()) {
		if (scene) {
			factory.updateScene(entities, prefab);
		} else {
			for (auto& entity: entities) {
				factory.updateEntity(entity, prefab->getEntityData());
			}
		}
	}

	/*
	const auto& deltas = prefab->getEntityDataDeltas();

	std::map<UUID, const EntityData*> dataMap;
	for (const auto& data: prefab->getEntityDatas()) {
		dataMap[data.getInstanceUUID()] = &data;
	}
	
	for (auto& entity: entities) {
		auto deltaIter = deltas.find(entity.getInstanceUUID());
		if (deltaIter != deltas.end()) {
			factory.updateEntity(entity, deltaIter->second);
		} else {
			auto dataIter = dataMap.find(entity.getInstanceUUID());
			if (dataIter != dataMap.end()) {
				factory.updateEntity(entity, *dataIter->second);
			} else {
				// Not found
			}
		}
	}*/
}

void EntityScene::PrefabObserver::markUpdated()
{
	assetVersion = prefab->getAssetVersion();
}

void EntityScene::PrefabObserver::addEntity(EntityRef entity, std::optional<int> index)
{
	std_ex::contains(entityIds, entity.getEntityId());
	entityIds.push_back(entity.getEntityId());
	if (index) {
		scene = true;
	}
}

const std::shared_ptr<const Prefab>& EntityScene::PrefabObserver::getPrefab() const
{
	return prefab;
}

EntityScene::PrefabObserver& EntityScene::getOrMakeObserver(const std::shared_ptr<const Prefab>& prefab)
{
	for (auto& o: prefabObservers) {
		if (o.getPrefab() == prefab) {
			return o;
		}
	}
	return prefabObservers.emplace_back(prefab);
}
