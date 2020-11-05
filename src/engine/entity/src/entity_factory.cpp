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
	if (!id.isEmpty()) {
		if (resources.exists<Prefab>(id)) {
			return resources.get<Prefab>(id);
		} else {
			Logger::logError("Prefab not found: \"" + id + "\".");
		}
	}

	return std::shared_ptr<const Prefab>();
}

EntityFactoryContext::EntityFactoryContext(World& world, Resources& resources, EntitySerialization::Type type, std::shared_ptr<const Prefab> _prefab, const EntityData* origEntityData)
	: world(&world)
{
	prefab = std::move(_prefab);
	configNodeContext.resources = &resources;
	configNodeContext.entityContext = this;
	configNodeContext.entitySerializationTypeMask = EntitySerialization::makeMask(type);

	if (prefab && origEntityData) {
		instancedEntityData = prefab->getEntityData().instantiateWithAsCopy(*origEntityData);
		entityData = &instancedEntityData;
	} else {
		entityData = origEntityData;
	}
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

bool EntityFactoryContext::needsNewContextFor(const EntityData& data) const
{
	const bool entityDataIsPrefabInstance = !data.getPrefab().isEmpty();
	const bool abandonPrefab = prefab && !data.getPrefabUUID().isValid();
	return entityDataIsPrefabInstance || abandonPrefab;
}

const EntityData& EntityFactoryContext::getRootEntityData() const
{
	return *entityData;
}

EntityRef EntityFactory::createEntity(const EntityData& data, EntityRef parent)
{
	const auto context = makeContext(data, EntityRef());
	return updateEntityNode(context->getRootEntityData(), parent, context);
}

void EntityFactory::updateEntity(EntityRef& entity, const EntityData& data)
{
	const auto context = makeContext(data, entity);
	updateEntityNode(context->getRootEntityData(), EntityRef(), context);
}

void EntityFactory::updateScene(std::vector<EntityRef>& entities, const std::shared_ptr<const Prefab>& scene, EntitySerialization::Type sourceType)
{
	// TODO
}

std::shared_ptr<EntityFactoryContext> EntityFactory::makeContext(const EntityData& data, EntityRef existing)
{
	// Create context
	auto context = std::make_shared<EntityFactoryContext>(world, resources, EntitySerialization::Type::Prefab, getPrefab(data.getPrefab()), &data);

	// Crawl entity
	if (existing.isValid()) {
		collectExistingEntities(existing, *context);
	}
	
	// Create entities
	preInstantiateEntities(context->getRootEntityData(), *context, 0);

	return context;
}

EntityRef EntityFactory::updateEntityNode(const EntityData& data, EntityRef parent, const std::shared_ptr<EntityFactoryContext>& context)
{
	auto entity = getEntity(data, *context, false);
	assert(entity.isValid());

	entity.setParent(parent);
	updateEntityComponents(entity, data, *context);
	updateEntityChildren(entity, data, context);
	
	return entity;
}

void EntityFactory::updateEntityComponents(EntityRef entity, const EntityData& data, const EntityFactoryContext& context)
{
	if (entity.getNumComponents() != 0) {
		// TODO: only remove components that are missing?
		entity.removeAllComponents();
	}

	const auto func = world.getCreateComponentFunction();
	for (const auto& [componentName, componentData]: data.getComponents()) {
		func(context, componentName, entity, componentData);
	}
}

void EntityFactory::updateEntityChildren(EntityRef entity, const EntityData& data, const std::shared_ptr<EntityFactoryContext>& context)
{
	if (!entity.getRawChildren().empty()) {
		// Delete old children that are no longer present
		const auto& newChildren = data.getChildren();
		std::vector<EntityRef> toDelete;
		for (auto c: entity.getChildren()) {
			const auto& uuid = c.getInstanceUUID();
			if (!std_ex::contains_if(newChildren, [&] (const EntityData& c) { return c.getInstanceUUID() == uuid; })) {
				toDelete.push_back(c);
			}
		}
		for (auto& c: toDelete) {
			world.destroyEntity(c);
		}
	}
	
	// Update children
	for (const auto& child: data.getChildren()) {
		if (context->needsNewContextFor(child)) {
			const auto newContext = makeContext(child, EntityRef());
			updateEntityNode(newContext->getRootEntityData(), entity, newContext);
		} else {
			updateEntityNode(child, entity, context);
		}
	}
}

void EntityFactory::preInstantiateEntities(const EntityData& data, EntityFactoryContext& context, int depth)
{
	instantiateEntity(data, context, depth == 0);
	
	for (const auto& child: data.getChildren()) {
		preInstantiateEntities(child, context, depth + 1);
	}
}

EntityRef EntityFactory::instantiateEntity(const EntityData& data, EntityFactoryContext& context, bool allowWorldLookup)
{
	const auto existing = getEntity(data, context, allowWorldLookup);
	if (existing.isValid()) {
		return existing;
	}
	
	const bool instantiatingFromPrefab = !!context.getPrefab();
	auto entity = world.createEntity(data.getInstanceUUID(), data.getName(), {}, instantiatingFromPrefab, data.getPrefabUUID());
	if (instantiatingFromPrefab) {
		entity.setPrefab(context.getPrefab());
	}

	context.addEntity(entity);

	return entity;
}

void EntityFactory::collectExistingEntities(EntityRef entity, EntityFactoryContext& context)
{
	context.addEntity(entity);
	
	for (const auto c: entity.getChildren()) {
		collectExistingEntities(c, context);
	}
}

EntityRef EntityFactory::getEntity(const EntityData& data, EntityFactoryContext& context, bool allowWorldLookup)
{
	Expects(data.getInstanceUUID().isValid());
	const auto result = context.getEntity(data.getInstanceUUID(), false);
	if (result.isValid()) {
		return result;
	}

	if (allowWorldLookup) {
		auto worldResult = world.findEntity(data.getInstanceUUID(), true);
		if (worldResult) {
			context.addEntity(*worldResult); // Should this be added to the context?
			return *worldResult;
		}
	}

	return EntityRef();
}
