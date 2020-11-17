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

const EntityData& Prefab::getEntityData() const
{
	if (entityDatas.size() != 1) {
		throw Exception("Prefab \"" + getAssetId() + "\" does not contain exactly one element.", HalleyExceptions::Entity);
	}
	return entityDatas[0];
}

const std::vector<EntityData>& Prefab::getEntityDatas() const
{
	return entityDatas;
}

std::map<UUID, const EntityData*> Prefab::getEntityDataMap() const
{
	std::map<UUID, const EntityData*> dataMap;
	for (const auto& data: entityDatas) {
		dataMap[data.getInstanceUUID()] = &data;
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
	return entityDatas.at(0).getName();
}

void Prefab::loadEntityData()
{
	entityDatas = makeEntityDatas();
}

std::vector<EntityData> Prefab::makeEntityDatas() const
{
	std::vector<EntityData> result;
	result.emplace_back(config.getRoot(), true);
	return result;
}

Prefab::Deltas Prefab::generatePrefabDeltas(const Prefab& newPrefab) const
{
	Deltas result;
	result.entitiesModified[entityDatas.at(0).getPrefabUUID()] = EntityDataDelta(entityDatas.at(0), newPrefab.entityDatas.at(0));
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

String Scene::getPrefabName() const
{
	return "Scene";
}

std::vector<EntityData> Scene::makeEntityDatas() const
{
	const auto& seq = config.getRoot().asSequence();
	std::vector<EntityData> result;
	result.reserve(seq.size());
	for (const auto& s: seq) {
		result.emplace_back(s, false);
	}
	return result;
}

Scene::Deltas Scene::generateSceneDeltas(const Scene& newScene) const
{
	Deltas result;
	
	// Move old entity data
	std::map<UUID, const EntityData*> oldDatas;
	for (const auto& data: entityDatas) {
		oldDatas[data.getInstanceUUID()] = &data;
	}

	// Modified
	for (const auto& data: newScene.entityDatas) {
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
