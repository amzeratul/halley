#include "prefab.h"
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

	prefab->entityData = EntityData(prefab->getRoot());

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
	entityData = EntityData();
}

const EntityData& Prefab::getEntityData() const
{
	return entityData;
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

void Scene::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<Scene&>(resource));
	updateRoot();
}

void Scene::makeDefault()
{
	getRoot() = ConfigNode(ConfigNode::SequenceType());
}
