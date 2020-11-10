#include "prefab.h"

#include "entity_data_delta.h"
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
	*this = std::move(dynamic_cast<Prefab&>(resource));
	updateRoot();
}

void Prefab::makeDefault()
{
	getRoot() = ConfigNode(ConfigNode::MapType());
	loadEntityData();
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

const std::map<UUID, EntityDataDelta>& Prefab::getEntityDataDeltas() const
{
	return deltas;
}

void Prefab::loadEntityData()
{
	/*
	// Move old entity data
	std::map<UUID, EntityData> oldDatas;
	for (auto& data: entityDatas) {
		oldDatas[data.getInstanceUUID()] = std::move(data);
	}
	*/
	
	// Get new entities
	entityDatas = makeEntityDatas();

	/*
	// Generate deltas
	deltas.clear();
	for (const auto& data: entityDatas) {
		const auto oldIter = oldDatas.find(data.getInstanceUUID());
		if (oldIter != oldDatas.end()) {
			deltas[data.getInstanceUUID()] = EntityDataDelta(oldIter->second, data);
		}
	}
	*/
}

std::vector<EntityData> Prefab::makeEntityDatas() const
{
	std::vector<EntityData> result;
	result.emplace_back(getRoot(), true);
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

void Scene::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<Scene&>(resource));
	updateRoot();
}

void Scene::makeDefault()
{
	getRoot() = ConfigNode(ConfigNode::SequenceType());
	loadEntityData();
}

std::vector<EntityData> Scene::makeEntityDatas() const
{
	const auto& seq = root.asSequence();
	std::vector<EntityData> result;
	result.reserve(seq.size());
	for (const auto& s: seq) {
		result.emplace_back(s, false);
	}
	return result;
}
