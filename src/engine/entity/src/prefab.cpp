#include "prefab.h"

#include "entity_data_delta.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/resources/resource_data.h"

using namespace Halley;

std::unique_ptr<Prefab> Prefab::loadResource(ResourceLoader& loader)
{
	auto data = loader.getStatic(false);
	if (!data) {
		return {};
	}
	
	auto prefab = std::make_unique<Prefab>();
	Deserializer::fromBytes(*prefab, data->getSpan());
	prefab->loadEntityData();

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
	config.getRoot() = ConfigNode(ConfigNode::MapType());
	loadEntityData();
}

void Prefab::serialize(Serializer& s) const
{
	s << config;
	s << gameData;
}

void Prefab::deserialize(Deserializer& s)
{
	s >> config;
	s >> gameData;
}

void Prefab::parseYAML(gsl::span<const gsl::byte> yaml)
{
	YAMLConvert::parseConfig(config, yaml);
}

String Prefab::toYAML() const
{
	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = {{ "name", "uuid", "components", "children" }};
	return YAMLConvert::generateYAML(config, options);
}

bool Prefab::isScene() const
{
	return false;
}

EntityData& Prefab::getEntityData()
{
	return entityData;
}

const EntityData& Prefab::getEntityData() const
{
	return entityData;
}

gsl::span<const EntityData> Prefab::getEntityDatas() const
{
	return gsl::span<const EntityData>(&entityData, 1);
}

gsl::span<EntityData> Prefab::getEntityDatas()
{
	return gsl::span<EntityData>(&entityData, 1);
}

std::map<UUID, const EntityData*> Prefab::getEntityDataMap() const
{
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

ConfigNode& Prefab::getEntityNodeRoot()
{
	return config.getRoot();
}

ConfigNode& Prefab::getGameData(const String& key)
{
	gameData.getRoot().ensureType(ConfigNodeType::Map);
	auto& map = gameData.getRoot().asMap();
	const auto iter = map.find(key);
	if (iter == map.end()) {
		auto [newIter, inserted] = map.insert(std::make_pair(key, ConfigNode::MapType()));
		return newIter->second;
	} else {
		return iter->second;
	}
}

const ConfigNode* Prefab::tryGetGameData(const String& key) const
{
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
	return entityData.getName();
}

EntityData* Prefab::findEntityData(const UUID& uuid)
{
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
	return std::make_shared<Prefab>(*this);
}

void Prefab::loadEntityData()
{
	entityData = makeEntityData();
}

EntityData Prefab::makeEntityData() const
{
	return EntityData(config.getRoot(), true);
}

Prefab::Deltas Prefab::generatePrefabDeltas(const Prefab& newPrefab) const
{
	Deltas result;
	result.entitiesModified[entityData.getPrefabUUID()] = EntityDataDelta(entityData, newPrefab.entityData);
	return result;
}

std::unique_ptr<Scene> Scene::loadResource(ResourceLoader& loader)
{
	auto data = loader.getStatic(false);
	if (!data) {
		return {};
	}
	
	auto scene = std::make_unique<Scene>();
	Deserializer::fromBytes(*scene, data->getSpan());
	scene->loadEntityData();

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
	config.getRoot() = ConfigNode(ConfigNode::SequenceType());
	loadEntityData();
}

gsl::span<const EntityData> Scene::getEntityDatas() const
{
	return entityData.getChildren();
}

gsl::span<EntityData> Scene::getEntityDatas()
{
	return entityData.getChildren();
}

String Scene::getPrefabName() const
{
	return "Scene";
}

std::shared_ptr<Prefab> Scene::clone() const
{
	return std::make_shared<Scene>(*this);
}

EntityData Scene::makeEntityData() const
{
	EntityData result;
	result.setSceneRoot(true);
	const auto& seq = config.getRoot().asSequence();
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
