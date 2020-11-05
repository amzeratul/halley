#include "entity_factory.h"


#include "component_reflector.h"
#include "entity_scene.h"
#include "halley/support/logger.h"
#include "world.h"
#include "registry.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/core/resources/resources.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

EntityFactory::EntityFactory(World& world, Resources& resources)
	: world(world)
	, resources(resources)
{
}

EntityFactory::~EntityFactory()
{
}

EntityRef EntityFactory::createEntity(const String& prefabName)
{
	EntityData data(UUID::generate());
	data.setPrefab(prefabName);
	return createEntity(data);
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
	const auto result = getEntity(uuid, true);
	if (result.isValid()) {
		return result.getEntityId();
	}
	Logger::logError("Couldn't find entity with UUID " + uuid.toString() + " while instantiating entity.");
	return EntityId();
}

void EntityFactoryContext::addEntity(EntityRef entity)
{
	entities.push_back(entity);
}

EntityRef EntityFactoryContext::getEntity(const UUID& uuid, bool allowPrefabUUID) const
{
	if (!uuid.isValid()) {
		return EntityRef();
	}
	
	for (const auto& e: entities) {
		if (e.getInstanceUUID() == uuid || (allowPrefabUUID && e.getPrefabUUID() == uuid)) {
			return e;
		}
	}
	return EntityRef();
}

EntityRef EntityFactory::createEntity(const EntityData& data, EntityRef parent)
{
	return createEntityTree(data, parent, {});
}

EntityRef EntityFactory::createEntityTree(const EntityData& data, EntityRef parent, const std::shared_ptr<EntityFactoryContext>& context)
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
				preInstantiateEntities(instanceData, *newContext);
				return createEntityNode(instanceData, parent, newContext);
			}
		}

		// Just instantiate the data given
		preInstantiateEntities(data, *newContext);
		return createEntityNode(data, parent, newContext);
	} else {
		// Forward old context
		return createEntityNode(data, parent, context);
	}
}

EntityRef EntityFactory::createEntityNode(const EntityData& data, EntityRef parent, const std::shared_ptr<EntityFactoryContext>& context)
{
	auto entity = instantiateEntity(data, *context);
	entity.setParent(parent);
	
	const auto func = world.getCreateComponentFunction();
	for (const auto& [componentName, componentData]: data.getComponents()) {
		func(*context, componentName, entity, componentData);
	}

	for (const auto& child: data.getChildren()) {
		createEntityTree(child, entity, context);
	}

	return entity;
}

EntityRef EntityFactory::instantiateEntity(const EntityData& data, EntityFactoryContext& context)
{
	Expects(data.getInstanceUUID().isValid());
	
	const auto existing = context.getEntity(data.getInstanceUUID(), false);
	if (existing.isValid()) {
		return existing;
	}

	const auto inWorld = world.findEntity(data.getInstanceUUID(), true);
	if (inWorld) {
		return inWorld.value();
	}
	
	const bool instantiatingFromPrefab = !!context.getPrefab();
	auto entity = world.createEntity(data.getInstanceUUID(), data.getName(), {}, instantiatingFromPrefab, data.getPrefabUUID());
	if (instantiatingFromPrefab) {
		entity.setPrefab(context.getPrefab());
	}

	context.addEntity(entity);

	return entity;
}

void EntityFactory::preInstantiateEntities(const EntityData& data, EntityFactoryContext& context)
{
	instantiateEntity(data, context);
	
	for (const auto& child: data.getChildren()) {
		preInstantiateEntities(child, context);
	}
}

void EntityFactory::updateScene(std::vector<EntityRef>& entities, const std::shared_ptr<const Prefab>& scene, EntitySerialization::Type sourceType)
{
	// TODO
}

void EntityFactory::updateEntity(EntityRef& entity, const EntityData& data)
{
	//createEntity(data);
}

/*
void EntityFactory::updateEntityNode(EntityRef& entity, const EntityData& data,	const std::shared_ptr<EntityFactoryContext>& context)
{
	const auto func = world.getCreateComponentFunction();
	for (const auto& [componentName, componentData]: data.getComponents()) {
		func(*context, componentName, entity, componentData);
	}
	// TODO: removed components

	std::set<UUID> childrenPresent;
	std::vector<EntityRef> toRemove;
	for (auto childEntity: entity.getChildren()) {
		childrenPresent.insert(childEntity.getInstanceUUID());
		const auto* childData = data.tryGetInstanceUUID(childEntity.getInstanceUUID());
		if (childData) {
			// Update existing child
			updateEntityTree(childEntity, *childData, context);
		} else {
			toRemove.push_back(childEntity);
		}
	}

	// Remove old
	for (auto& e: toRemove) {
		world.destroyEntity(e);
	}

	// New children
	for (const auto& childData: data.getChildren()) {
		if (!std_ex::contains(childrenPresent, childData.getInstanceUUID())) {
			createEntityTree(childData, entity, context);
		}
	}
}
*/
