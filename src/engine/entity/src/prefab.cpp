#include "prefab.h"

#include "entity_data_delta.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/core/resources/resources.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/resources/resource_data.h"
#include "halley/support/logger.h"

using namespace Halley;

std::shared_ptr<Prefab> Prefab::loadResource(ResourceLoader& loader)
{
	auto prefab = std::make_shared<Prefab>();
	prefab->startLoading();
	auto& res = loader.getResources();
	
	loader.getAsync().then([prefab, &res] (std::unique_ptr<ResourceDataStatic> dataStatic)
	{
		Deserializer::fromBytes(*prefab, dataStatic->getSpan());
		prefab->doneLoading();
		prefab->preloadDependencies(res);
	});
	
	return prefab;
}

void Prefab::reload(Resource&& resource)
{
	auto& prefab = dynamic_cast<Prefab&>(resource);
	auto newDeltas = generatePrefabDeltas(prefab);
	*this = std::move(prefab);
	deltas = std::move(newDeltas);
}

void Prefab::makeDefault()
{
	entityData.setInstanceUUID(UUID::generate());
}

void Prefab::serialize(Serializer& s) const
{
	s << entityData;
	s << gameData;
}

void Prefab::deserialize(Deserializer& s)
{
	s >> entityData;
	s >> gameData;
	entityData.setSceneRoot(isScene());
}

void Prefab::parseYAML(gsl::span<const gsl::byte> yaml)
{
	ConfigFile config;
	YAMLConvert::parseConfig(config, yaml);
	parseConfigNode(std::move(config.getRoot()));
}

String Prefab::toYAML() const
{
	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = {{ "name", "icon", "uuid", "prefab", "components", "children" }};
	return YAMLConvert::generateYAML(toConfigNode(), options);
}

void Prefab::parseConfigNode(ConfigNode node)
{
	if (node.getType() == ConfigNodeType::Map && node.hasKey("entity")) {
		entityData = makeEntityData(node["entity"]);
		gameData.getRoot() = std::move(node["game"]);
	} else {
		// Legacy
		entityData = makeEntityData(node);
		gameData.getRoot() = ConfigNode();
	}

	entityData.setSceneRoot(isScene());
}

ConfigNode Prefab::toConfigNode() const
{
	ConfigNode::MapType result;
	result["entity"] = entityToConfigNode();
	result["game"] = ConfigNode(gameData.getRoot());
	return result;
}

ConfigNode Prefab::entityToConfigNode() const
{
	return entityData.toConfigNode(false);
}

bool Prefab::isScene() const
{
	return false;
}

EntityData& Prefab::getEntityData()
{
	waitForLoad();
	return entityData;
}

const EntityData& Prefab::getEntityData() const
{
	waitForLoad();
	return entityData;
}

gsl::span<const EntityData> Prefab::getEntityDatas() const
{
	waitForLoad();
	return gsl::span<const EntityData>(&entityData, 1);
}

gsl::span<EntityData> Prefab::getEntityDatas()
{
	waitForLoad();
	return gsl::span<EntityData>(&entityData, 1);
}

std::map<UUID, const EntityData*> Prefab::getEntityDataMap() const
{
	waitForLoad();
	std::map<UUID, const EntityData*> dataMap;

	if (isScene()) {
		for (const auto& data: entityData.getChildren()) {
			dataMap[data.getInstanceUUID()] = &data;
		}		
	} else {
		dataMap[entityData.getInstanceUUID()] = &entityData;
	}
	
	return dataMap;
}

const std::map<UUID, EntityDataDelta>& Prefab::getEntitiesModified() const
{
	return deltas.entitiesModified;
}

const std::set<UUID>& Prefab::getEntitiesAdded() const
{
	return deltas.entitiesAdded;
}

const std::set<UUID>& Prefab::getEntitiesRemoved() const
{
	return deltas.entitiesRemoved;
}

void Prefab::setGameData(const String& key, ConfigNode data)
{
	gameData.getRoot().ensureType(ConfigNodeType::Map);
	gameData.getRoot()[key] = std::move(data);
}

void Prefab::removeGameData(const String& key)
{
	if (gameData.getRoot().getType() == ConfigNodeType::Map) {
		gameData.getRoot().asMap().erase(key);
	}
}

ConfigNode& Prefab::getGameData(const String& key)
{
	waitForLoad();

	gameData.getRoot().ensureType(ConfigNodeType::Map);
	auto& map = gameData.getRoot().asMap();
	const auto iter = map.find(key);
	if (iter == map.end()) {
		auto [newIter, inserted] = map.insert(std::make_pair<String, ConfigNode>(String(key), ConfigNode::MapType()));
		return newIter->second;
	} else {
		return iter->second;
	}
}

const ConfigNode* Prefab::tryGetGameData(const String& key) const
{
	waitForLoad();

	if (gameData.getRoot().getType() != ConfigNodeType::Map) {
		return nullptr;
	}

	auto& map = gameData.getRoot().asMap();
	const auto iter = map.find(key);
	if (iter == map.end()) {
		return nullptr;
	} else {
		return &iter->second;
	}
}

String Prefab::getPrefabName() const
{
	waitForLoad();
	return entityData.getName();
}

String Prefab::getPrefabIcon() const
{
	waitForLoad();
	return entityData.getIcon();
}

EntityData* Prefab::findEntityData(const UUID& uuid)
{
	waitForLoad();
	if (!uuid.isValid()) {
		if (isScene()) {
			return &entityData;
		} else {
			return nullptr;
		}
	}
	return entityData.tryGetInstanceUUID(uuid);
}

std::shared_ptr<Prefab> Prefab::clone() const
{
	waitForLoad();
	return std::make_shared<Prefab>(*this);
}

void Prefab::preloadDependencies(Resources& resources) const
{
	for (const auto& data: getEntityDatas()) {
		doPreloadDependencies(data, resources);
	}
}

void Prefab::doPreloadDependencies(const EntityData& data, Resources& resources) const
{
	if (!data.getPrefab().isEmpty()) {
		resources.preload<Prefab>(data.getPrefab());
	}
	for (auto c: data.getChildren()) {
		doPreloadDependencies(c, resources);
	}
}

EntityData Prefab::makeEntityData(const ConfigNode& node) const
{
	return EntityData(node, true);
}

Prefab::Deltas Prefab::generatePrefabDeltas(const Prefab& newPrefab) const
{
	Deltas result;
	result.entitiesModified[entityData.getPrefabUUID()] = EntityDataDelta(entityData, newPrefab.entityData);
	return result;
}

std::shared_ptr<Scene> Scene::loadResource(ResourceLoader& loader)
{
	auto scene = std::make_shared<Scene>();
	scene->startLoading();
	auto& resources = loader.getResources();
	
	loader.getAsync().then([scene, &resources] (std::unique_ptr<ResourceDataStatic> dataStatic)
	{
		Deserializer::fromBytes(*scene, dataStatic->getSpan());
		scene->doneLoading();
		scene->preloadDependencies(resources);
	});
	
	return scene;
}

bool Scene::isScene() const
{
	return true;
}

void Scene::reload(Resource&& resource)
{
	auto& scene = dynamic_cast<Scene&>(resource);
	auto newDeltas = generateSceneDeltas(scene);
	*this = std::move(scene);
	deltas = std::move(newDeltas);
}

void Scene::makeDefault()
{
	
}

gsl::span<const EntityData> Scene::getEntityDatas() const
{
	waitForLoad();
	return entityData.getChildren();
}

gsl::span<EntityData> Scene::getEntityDatas()
{
	waitForLoad();
	return entityData.getChildren();
}

String Scene::getPrefabName() const
{
	return "Scene";
}

std::shared_ptr<Prefab> Scene::clone() const
{
	waitForLoad();
	return std::make_shared<Scene>(*this);
}

ConfigNode Scene::entityToConfigNode() const
{
	waitForLoad();
	ConfigNode::SequenceType result;
	for (auto& c: entityData.getChildren()) {
		result.emplace_back(c.toConfigNode(false));
	}
	return ConfigNode(std::move(result));
}

EntityData Scene::makeEntityData(const ConfigNode& node) const
{
	waitForLoad();
	EntityData result;
	const auto& seq = node.asSequence();
	result.getChildren().reserve(seq.size());
	for (const auto& s: seq) {
		result.getChildren().emplace_back(s, false);
	}
	return result;
}

Scene::Deltas Scene::generateSceneDeltas(const Scene& newScene) const
{
	Deltas result;
	
	// Mapping of old entity data
	std::map<UUID, const EntityData*> oldDatas;
	for (const auto& data: entityData.getChildren()) {
		oldDatas[data.getInstanceUUID()] = &data;
	}

	// Modified
	for (const auto& data: newScene.entityData.getChildren()) {
		const auto uuid = data.getInstanceUUID();
		const auto oldIter = oldDatas.find(uuid);
		if (oldIter != oldDatas.end()) {
			// Update
			auto delta = EntityDataDelta(*oldIter->second, data);
			if (delta.hasChange()) {
				result.entitiesModified[uuid] = std::move(delta);
			}
			oldDatas.erase(uuid);
		} else {
			// Added
			result.entitiesAdded.insert(uuid);
		}
	}

	// Removed
	for (const auto& old: oldDatas) {
		result.entitiesRemoved.insert(old.first);
	}

	return result;
}
