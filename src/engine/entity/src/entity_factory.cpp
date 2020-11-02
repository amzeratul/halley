#include "entity_factory.h"


#include "component_reflector.h"
#include "entity_scene.h"
#include "halley/support/logger.h"
#include "world.h"
#include "registry.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/core/resources/resources.h"

using namespace Halley;

EntityFactory::EntityFactory(World& world, Resources& resources)
	: world(world)
	, resources(resources)
	, context(makeContext())
{
	dummyPrefab = ConfigNode(ConfigNode::MapType());
	dummyPrefab["uuid"] = "00000000-0000-0000-0000-000000000000";
	dummyPrefab["name"] = "Missing";
}

EntityFactory::~EntityFactory()
{
}

EntityRef EntityFactory::createEntity(const char* prefabName)
{
	return createEntity(getPrefab(prefabName));
}

EntityRef EntityFactory::createEntity(const String& prefabName)
{
	return createEntity(getPrefab(prefabName));
}

EntityRef EntityFactory::createEntity(const std::shared_ptr<const Prefab>& prefab)
{
	if (!prefab) {
		Logger::logWarning("Missing prefab");
		return EntityRef();
	} else {
		return createEntity(prefab->getEntityData());
	}
}

EntityScene EntityFactory::createScene(const std::shared_ptr<const Prefab>& prefab)
{
	EntityScene curScene;
	int i = 0;
	for (const auto& entityData: prefab->getEntityDatas()) {
		auto entity = createEntity(entityData);
		curScene.addPrefabReference(prefab, entity, i++);
		curScene.addRootEntity(entity);	
	}
	return curScene;
}

void EntityFactory::updateScene(std::vector<EntityRef>& entities, const std::shared_ptr<const Prefab>& scene, EntitySerialization::Type sourceType)
{
	// TODO
}

EntityData EntityFactory::serializeEntity(EntityRef entity, const SerializationOptions& options, bool canStoreParent)
{
	EntityData result;

	// Properties
	result.setName(entity.getName());
	result.setInstanceUUID(entity.getInstanceUUID());
	result.setPrefabUUID(entity.getPrefabUUID());

	// Components
	ConfigNodeSerializationContext serializeContext = makeContext();
	serializeContext.entitySerializationTypeMask = makeMask(options.type);
	for (auto [componentId, component]: entity) {
		auto& reflector = getComponentReflector(componentId);
		result.getComponents().emplace_back(reflector.getName(), reflector.serialize(serializeContext, *component));
	}

	// Children
	for (const auto child: entity.getChildren()) {
		if (child.isSerializable()) {
			if (options.serializeAsStub && options.serializeAsStub(child)) {
				// Store just a stub
				result.getChildren().emplace_back(child.getInstanceUUID());
			} else {
				result.getChildren().push_back(serializeEntity(child, options, false));
			}
		}
	}

	// Parent
	if (canStoreParent) {
		auto parent = entity.tryGetParent();
		if (parent) {
			result.setParentUUID(parent->getInstanceUUID());
		}
	}
	
	return result;
}

std::shared_ptr<const Prefab> EntityFactory::getPrefab(const String& id) const
{
	return resources.exists<Prefab>(id) ? resources.get<Prefab>(id) : std::shared_ptr<const Prefab>();
}

void EntityFactory::startContext(EntitySerialization::Type sourceType)
{
	// Warning: this makes this whole class not thread safe
	context.entityContext->clear();
	//context.entitySerializationTypeMask = makeMask(sourceType); // TODO
	context.entitySerializationTypeMask = makeMask(EntitySerialization::Type::Prefab, EntitySerialization::Type::SaveData);
}

ConfigNodeSerializationContext EntityFactory::makeContext() const
{
	ConfigNodeSerializationContext context;
	context.resources = &resources;
	context.entityContext = std::make_shared<EntitySerializationContext>(world);
	return context;
}

EntitySerializationContext::EntitySerializationContext(World& world)
	: world(world)
{
}

void EntitySerializationContext::clear()
{
	uuids.clear();
}

EntityRef EntityFactory::createEntity(const EntityData& data, EntityRef parent)
{
	return createEntity(data, parent, {});
}

EntityRef EntityFactory::createEntity(const EntityData& data, EntityRef parent, const std::shared_ptr<const Prefab>& prevPrefab)
{
	if (!data.getPrefab().isEmpty()) {
		const auto newPrefab = getPrefab(data.getPrefab());
		if (newPrefab) {
			// New prefab found, generate tree based on it
			const auto instanceData = newPrefab->getEntityData().instantiateWithAsCopy(data);
			return doCreateEntity(instanceData, parent, newPrefab);
		} else {
			Logger::logError("Prefab \"" + data.getPrefab() + "\" not found while instantiating entity.");
			return doCreateEntity(data, parent, {});
		}
	} else {
		return doCreateEntity(data, parent, prevPrefab);
	}
}

EntityRef EntityFactory::doCreateEntity(const EntityData& data, EntityRef parent, const std::shared_ptr<const Prefab>& prefab)
{
	const bool instantiatingFromPrefab = prefab && data.getPrefabUUID().isValid();
	
	EntityRef entity = world.createEntity(data.getInstanceUUID(), data.getName(), parent, instantiatingFromPrefab, data.getPrefabUUID());
	if (instantiatingFromPrefab) {
		entity.setPrefab(prefab);
	}
	
	const auto func = world.getCreateComponentFunction();
	for (const auto& [componentName, componentData]: data.getComponents()) {
		func(*this, componentName, entity, componentData);
	}

	for (const auto& child: data.getChildren()) {
		createEntity(child, entity, prefab);
	}

	return entity;
}

void EntityFactory::updateEntity(EntityRef& entity, const EntityData& data)
{
	// TODO
}
