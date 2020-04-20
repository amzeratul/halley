#include "entity_scene.h"
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
	for (auto& entry: prefabObservers) {
		entry.update(factory);
	}
}

void EntityScene::addPrefabReference(const std::shared_ptr<const Prefab>& prefab, const EntityRef& entity, std::optional<int> index)
{
	getOrMakeObserver(prefab).entities.emplace_back(entity, index);
}

void EntityScene::addRootEntity(EntityRef entity)
{
	entities.emplace_back(entity);
}

EntityScene::ObservedEntity::ObservedEntity(EntityRef entity, std::optional<int> index)
	: entity(entity)
	, index(index)
{
}

EntityScene::PrefabObserver::PrefabObserver(std::shared_ptr<const ConfigFile> config)
	: config(std::move(config))
{
	assetVersion = this->config->getAssetVersion();
}

bool EntityScene::PrefabObserver::needsUpdate() const
{
	return assetVersion != config->getAssetVersion();
}

void EntityScene::PrefabObserver::update(EntityFactory& factory)
{
	assetVersion = config->getAssetVersion();
	for (auto& entity: entities) {
		// TODO
	}
}

EntityScene::PrefabObserver& EntityScene::getOrMakeObserver(const std::shared_ptr<const ConfigFile>& config)
{
	for (auto& o: prefabObservers) {
		if (o.config == config) {
			return o;
		}
	}
	return prefabObservers.emplace_back(config);
}
