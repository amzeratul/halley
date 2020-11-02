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
{
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
	const auto serializeContext = std::make_shared<EntityFactoryContext>(world, resources, options.type);
	for (auto [componentId, component]: entity) {
		auto& reflector = getComponentReflector(componentId);
		result.getComponents().emplace_back(reflector.getName(), reflector.serialize(serializeContext->getConfigNodeContext(), *component));
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

EntityFactoryContext::EntityFactoryContext(World& world, Resources& resources, EntitySerialization::Type type, std::shared_ptr<const Prefab> prefab)
	: world(&world)
{
	this->prefab = std::move(prefab);
	configNodeContext.resources = &resources;
	configNodeContext.entityContext = this;
	configNodeContext.entitySerializationTypeMask = EntitySerialization::makeMask(type);
}

EntityId EntityFactoryContext::getEntityIdFromUUID(const UUID& uuid) const
{
	// TODO
	return EntityId();
}

EntityRef EntityFactory::createEntity(const EntityData& data, EntityRef parent)
{
	return createEntityTree(data, parent, {});
}

EntityRef EntityFactory::createEntityTree(const EntityData& data, EntityRef parent, const std::shared_ptr<const EntityFactoryContext>& context)
{
	const bool entityDataIsPrefabInstance = !data.getPrefab().isEmpty();
	const bool abandonPrefab = context && context->getPrefab() && !data.getPrefabUUID().isValid();
	
	if (!context || entityDataIsPrefabInstance || abandonPrefab) {
		// Load prefab and create new context
		const auto prefab = entityDataIsPrefabInstance ? getPrefab(data.getPrefab()) : std::shared_ptr<const Prefab>();
		const auto newContext = std::make_shared<EntityFactoryContext>(world, resources, EntitySerialization::Type::Prefab, prefab);

		// Instantiate prefab
		if (entityDataIsPrefabInstance) {
			if (!prefab) {
				Logger::logError("Prefab \"" + data.getPrefab() + "\" not found while instantiating entity.");
			} else {
				const auto instanceData = prefab->getEntityData().instantiateWithAsCopy(data);
				return createEntityNode(instanceData, parent, newContext);
			}
		}

		// Just instantiate the data given
		return createEntityNode(data, parent, newContext);
	} else {
		// Forward old context
		return createEntityNode(data, parent, context);
	}
}

EntityRef EntityFactory::createEntityNode(const EntityData& data, EntityRef parent, const std::shared_ptr<const EntityFactoryContext>& context)
{
	const bool instantiatingFromPrefab = !!context->getPrefab();
	
	EntityRef entity = world.createEntity(data.getInstanceUUID(), data.getName(), parent, instantiatingFromPrefab, data.getPrefabUUID());
	if (instantiatingFromPrefab) {
		entity.setPrefab(context->getPrefab());
	}
	
	const auto func = world.getCreateComponentFunction();
	for (const auto& [componentName, componentData]: data.getComponents()) {
		func(*context, componentName, entity, componentData);
	}

	for (const auto& child: data.getChildren()) {
		createEntityTree(child, entity, context);
	}

	return entity;
}

void EntityFactory::updateEntity(EntityRef& entity, const EntityData& data)
{
	// TODO
}
