#include "entity_scene.h"

#include "entity_factory.h"
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
	getOrMakeObserver(prefab).addEntity(entity, index);
}

void EntityScene::addRootEntity(EntityRef entity)
{
	entities.emplace_back(entity);
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

	if (!entities.empty()) {
		if (isScene) {
			factory.updateScene(entities, config->getRoot());
		} else {
			for (auto& entity: entities) {
				factory.updateEntityTree(entity, config->getRoot());
			}
		}
	}
}

void EntityScene::PrefabObserver::addEntity(EntityRef entity, std::optional<int> index)
{
	entities.push_back(entity);
	if (index) {
		isScene = true;
	}
}

const std::shared_ptr<const ConfigFile>& EntityScene::PrefabObserver::getConfig() const
{
	return config;
}

EntityScene::PrefabObserver& EntityScene::getOrMakeObserver(const std::shared_ptr<const ConfigFile>& config)
{
	for (auto& o: prefabObservers) {
		if (o.getConfig() == config) {
			return o;
		}
	}
	return prefabObservers.emplace_back(config);
}
