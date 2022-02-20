#include "entity_scene.h"

#include <cassert>

#include "entity_factory.h"
#include "world.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
using namespace Halley;

EntityScene::EntityScene(bool allowReload, uint8_t worldPartition)
	: allowReload(allowReload)
	, worldPartition(worldPartition)
{
}

Vector<EntityId>& EntityScene::getEntities()
{
	return entities;
}

const Vector<EntityId>& EntityScene::getEntities() const
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

void EntityScene::update(EntityFactory& factory, IEntitySceneUpdateCallbacks* callbacks)
{
	// Collect all prefabs that changed
	for (auto& entry: prefabObservers) {
		if (entry.needsUpdate()) {
			entry.updateEntities(factory, *this, callbacks, worldPartition);
			entry.markUpdated();		
		}
	}

	// Update scenes
	for (auto& entry: sceneObservers) {
		if (entry.needsUpdate()) {
			entry.updateEntities(factory, *this, callbacks, worldPartition);
			entry.markUpdated();
		}
	}
}

void EntityScene::addPrefabReference(const std::shared_ptr<const Prefab>& prefab, const EntityRef& entity)
{
	if (allowReload) {
		getOrMakeObserver(prefab).addEntity(entity);
	}
}

void EntityScene::addRootEntity(EntityRef entity)
{
	entities.emplace_back(entity.getEntityId());
}

uint8_t EntityScene::getWorldPartition() const
{
	return worldPartition;
}

void EntityScene::validate(uint8_t worldPartition, World& world)
{
	for (const auto& id: entities) {
		auto e = world.tryGetEntity(id);
		if (e.isValid()) {
			if (e.getWorldPartition() != worldPartition) {
				Logger::logError("Entity \"" + e.getName() + "\" with id 0x" + toString(e.getEntityId().value, 16) + " and prefab " + e.getPrefabAssetId().value_or("<none>") + " is in world partition " + toString(int(e.getWorldPartition())) + " instead of " + toString(int(worldPartition)));
			}
			if (!e.isAlive()) {
				Logger::logError("Entity \"" + e.getName() + "\" is dead!");
			}
		}
	}
}

void EntityScene::destroyEntities(World& world)
{
	for (const auto id: entities) {
		auto e = world.tryGetEntity(id);
		if (e.isValid()) {
			if (e.getWorldPartition() != worldPartition) {
				Logger::logError("Unloading entity \"" + e.getName() + "\" from the wrong world partition! Entity at " + toString(int(e.getWorldPartition())) + ", worldPartition at " + toString(int(worldPartition)));
			}

			world.destroyEntity(e);
		}
	}
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

void EntityScene::PrefabObserver::updateEntities(EntityFactory& factory, EntityScene& scene, IEntitySceneUpdateCallbacks* callbacks, uint8_t worldPartition) const
{
	const auto& modified = prefab->getEntitiesModified();
	const auto& removed = prefab->getEntitiesRemoved();
	const auto& dataMap = prefab->getEntityDataMap();

	if (!prefab->isScene()) {
		assert(modified.size() == 1 && removed.empty());
	}

	// Modified entities
	for (auto& entity: getEntities(factory.getWorld())) {
		const auto& uuid = prefab->isScene() ? entity.getInstanceUUID() : entity.getPrefabUUID();
		
		auto deltaIter = modified.find(uuid);
		if (deltaIter != modified.end()) {
			// A simple delta is available for this entity, apply that
			factory.updateEntity(entity, deltaIter->second, static_cast<int>(EntitySerialization::Type::Prefab));
			if (callbacks) {
				callbacks->onEntityUpdated(entity, deltaIter->second, worldPartition);
			}
		} else if (removed.find(uuid) != removed.end()) {
			// Remove
			if (callbacks) {
				callbacks->onEntityRemoved(entity, worldPartition);
			}
			factory.getWorld().destroyEntity(entity);
		}
	}

	// Added
	for (const auto& uuid: prefab->getEntitiesAdded()) {
		auto dataIter = dataMap.find(uuid);
		if (dataIter != dataMap.end()) {
			// Create
			const auto entity = factory.createEntity(*dataIter->second, {}, &scene);
			if (callbacks) {
				callbacks->onEntityAdded(entity, *dataIter->second, worldPartition);
			}
		} else {
			// Not found
			Logger::logError("PrefabObserver::update error: UUID " + uuid.toString() + " not found in prefab " + prefab->getAssetId());
		}
	}
}

void EntityScene::PrefabObserver::markUpdated()
{
	assetVersion = prefab->getAssetVersion();
}

void EntityScene::PrefabObserver::addEntity(EntityRef entity)
{
	if (!std_ex::contains(entityIds, entity.getEntityId())) {
		entityIds.push_back(entity.getEntityId());
	}
}

const std::shared_ptr<const Prefab>& EntityScene::PrefabObserver::getPrefab() const
{
	return prefab;
}

Vector<EntityRef> EntityScene::PrefabObserver::getEntities(World& world) const
{
	Vector<EntityRef> entities;
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
