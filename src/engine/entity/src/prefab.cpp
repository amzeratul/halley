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

void Prefab::setGameData(const String& key, ConfigNode data)
{
	gameData.getRoot().ensureType(ConfigNodeType::Map);
	gameData.getRoot()[key] = std::move(data);
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

String Prefab::getPrefabIcon() const
{
	return entityData.getIcon();
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

std::unique_ptr<Scene> Scene::loadResource(ResourceLoader& loader)
{
	auto data = loader.getStatic(false);
	if (!data) {
		return {};
	}
	
	auto scene = std::make_unique<Scene>();
	Deserializer::fromBytes(*scene, data->getSpan());

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

ConfigNode Scene::entityToConfigNode() const
{
	ConfigNode::SequenceType result;
	for (auto& c: entityData.getChildren()) {
		result.emplace_back(c.toConfigNode(false));
	}
	return ConfigNode(std::move(result));
}

EntityData Scene::makeEntityData(const ConfigNode& node) const
{
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
