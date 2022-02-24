#include "entity_factory.h"

#include <cassert>


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

World& EntityFactory::getWorld()
{
	return world;
}

EntityRef EntityFactory::createEntity(const String& prefabName)
{
	EntityData data(UUID::generate());
	data.setPrefab(prefabName);
	return createEntity(data);
}

EntityScene EntityFactory::createScene(const std::shared_ptr<const Prefab>& prefab, bool allowReload, uint8_t worldPartition)
{
	EntityScene curScene(allowReload, worldPartition);
	for (const auto& entityData: prefab->getEntityDatas()) {
		auto entity = createEntity(entityData, EntityRef(), &curScene);
		curScene.addPrefabReference(prefab, entity);
		curScene.addRootEntity(entity);	
	}
	return curScene;
}

EntityData EntityFactory::serializeEntity(EntityRef entity, const SerializationOptions& options, bool canStoreParent)
{
	EntityData result;

	// Properties
	result.setName(entity.getName());
	result.setFlag(EntityData::Flag::NotSelectable, !entity.isSelectable());
	result.setInstanceUUID(entity.getInstanceUUID());
	result.setPrefabUUID(entity.getPrefabUUID());

	// Components
	const auto serializeContext = std::make_shared<EntityFactoryContext>(world, resources, EntitySerialization::makeMask(options.type), false);
	for (auto [componentId, component]: entity) {
		auto& reflector = getComponentReflector(componentId);
		result.getComponents().emplace_back(reflector.getName(), reflector.serialize(serializeContext->getEntitySerializationContext(), *component));
	}

	// Children
	for (const auto& child: entity.getChildren()) {
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

EntityDataDelta EntityFactory::serializeEntityAsDelta(EntityRef entity, const SerializationOptions& options, const EntityDataDelta::Options& deltaOptions, bool canStoreParent)
{
	auto entityData = serializeEntity(entity, options, canStoreParent);
	return entityDataToPrefabDelta(std::move(entityData), entity.getPrefab(), deltaOptions);
}

EntityDataDelta EntityFactory::entityDataToPrefabDelta(EntityData entityData, std::shared_ptr<const Prefab> prefab, const EntityDataDelta::Options& deltaOptions)
{
	if (prefab) {
		entityData.setPrefab(prefab->getAssetId());
		const auto* prefabData = prefab->getEntityData().tryGetPrefabUUID(entityData.getPrefabUUID());
		assert(prefabData);
		auto delta = EntityDataDelta(*prefabData, entityData, deltaOptions);
		delta.setPrefabUUID(entityData.getPrefabUUID());
		return delta;
	} else {
		//Logger::logInfo("Entity " + entity.getName() + " has no prefab associated with it.");
		return EntityDataDelta(entityData, deltaOptions);
	}
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

std::shared_ptr<const Prefab> EntityFactory::getPrefab(std::optional<EntityRef> entity, const IEntityData& data) const
{
	if (data.isDelta()) {
		assert(entity);
		const auto& prefabId = data.asEntityDataDelta().getPrefab();
		if (prefabId) {
			return getPrefab(prefabId.value());
		} else {
			return entity->getPrefab();
		}
	} else {
		return getPrefab(data.asEntityData().getPrefab());
	}
}

EntityFactoryContext::EntityFactoryContext(World& world, Resources& resources, int entitySerializationMask, bool update, std::shared_ptr<const Prefab> _prefab, const IEntityData* origEntityData, EntityScene* scene, EntityFactoryContext* parent, IDataInterpolatorSetRetriever* interpolators)
	: world(&world)
	, scene(scene)
	, parent(parent)
	, update(update)
{
	prefab = std::move(_prefab);
	entitySerializationContext.resources = &resources;
	entitySerializationContext.entityContext = this;
	entitySerializationContext.entitySerializationTypeMask = entitySerializationMask;
	entitySerializationContext.interpolators = interpolators;
	worldPartition = scene ? scene->getWorldPartition() : 0;

	if (origEntityData) {
		setEntityData(*origEntityData);
	}
}

EntityId EntityFactoryContext::getEntityIdFromUUID(const UUID& uuid) const
{
	const auto result = getEntity(uuid, true, true);
	if (result.isValid()) {
		return result.getEntityId();
	}
	Logger::logError("Couldn't find entity with UUID " + uuid.toString() + " while instantiating entity.");
	return EntityId();
}

void EntityFactoryContext::addEntity(EntityRef entity)
{
	if (entity.getWorldPartition() != getWorldPartition()) {
		Logger::logError("Loading entity \"" + entity.getName() + "\" with the wrong world partition! Entity at " + toString(int(entity.getWorldPartition())) + ", scene at " + toString(int(getWorldPartition())));
	}
	entities.push_back(entity);
}

EntityRef EntityFactoryContext::getEntity(const UUID& uuid, bool allowPrefabUUID, bool allowWorldLookup) const
{
	if (!uuid.isValid()) {
		return EntityRef();
	}
	
	for (const auto& e: entities) {
		if (e.getInstanceUUID() == uuid || (allowPrefabUUID && e.getPrefabUUID() == uuid)) {
			return e;
		}
	}

	if (parent) {
		return parent->getEntity(uuid, allowPrefabUUID, allowWorldLookup);
	} else {
		return allowWorldLookup ? world->findEntity(uuid, true).value_or(EntityRef()) : EntityRef();
	}
}

bool EntityFactoryContext::needsNewContextFor(const EntityData& data) const
{
	const bool entityDataIsPrefabInstance = !data.getPrefab().isEmpty();
	const bool abandonPrefab = prefab && !data.getPrefabUUID().isValid();
	return entityDataIsPrefabInstance || abandonPrefab;
}

bool EntityFactoryContext::isUpdateContext() const
{
	return update;
}

const IEntityData& EntityFactoryContext::getRootEntityData() const
{
	return *entityData;
}

EntityScene* EntityFactoryContext::getScene() const
{
	return scene;
}

uint8_t EntityFactoryContext::getWorldPartition() const
{
	return worldPartition;
}

void EntityFactoryContext::setWorldPartition(uint8_t partition)
{
	worldPartition = partition;
}

void EntityFactoryContext::setCurrentEntity(EntityRef entity)
{
	curEntity = entity;
}

EntityRef EntityFactoryContext::getCurrentEntity() const
{
	return curEntity;
}

void EntityFactoryContext::setEntityData(const IEntityData& iData)
{
	if (prefab) {
		if (iData.isDelta()) {
			const auto& newPrefab = iData.asEntityDataDelta().getPrefab();
			if (newPrefab && newPrefab != prefab->getAssetId()) {
				Logger::logWarning("Changing prefab in EntityFactoryContext::setEntityData, this will probably not work correctly");
			}
			entityData = &iData;
		} else {
			instancedEntityData = prefab->getEntityData().instantiateWithAsCopy(iData.asEntityData());
			entityData = &instancedEntityData;
		}
	} else {
		entityData = &iData;
	}
}

void EntityFactoryContext::notifyEntity(const EntityRef& entity) const
{
	if (scene && prefab) {
		scene->addPrefabReference(prefab, entity);
	}
}

EntityRef EntityFactory::createEntity(const EntityData& data, EntityRef parent, EntityScene* scene)
{
	const auto mask = makeMask(EntitySerialization::Type::Prefab, EntitySerialization::Type::SaveData);
	const auto context = makeContext(data, {}, scene, false, mask);
	const auto entity = tryGetEntity(data.getInstanceUUID(), *context, false);
	updateEntityNode(context->getRootEntityData(), entity, parent, context);
	return entity;
}

void EntityFactory::updateEntity(EntityRef& entity, const IEntityData& data, int serializationMask, EntityScene* scene, IDataInterpolatorSetRetriever* interpolators)
{
	Expects(entity.isValid());
	const auto context = makeContext(data, entity, scene, true, serializationMask, nullptr, interpolators);
	updateEntityNode(context->getRootEntityData(), entity, {}, context);
}

std::shared_ptr<EntityFactoryContext> EntityFactory::makeContext(const IEntityData& data, std::optional<EntityRef> existing, EntityScene* scene, bool updateContext, int serializationMask, EntityFactoryContext* parent, IDataInterpolatorSetRetriever* interpolators)
{
	auto context = std::make_shared<EntityFactoryContext>(world, resources, serializationMask, updateContext, getPrefab(existing, data), &data, scene, parent, interpolators);

	if (existing) {
		context->notifyEntity(existing.value());
		if (updateContext) {
			context->setWorldPartition(existing->getWorldPartition());
			collectExistingEntities(existing.value(), *context);
		}
	}
	
	preInstantiateEntities(context->getRootEntityData(), *context, 0);

	return context;
}

std::shared_ptr<EntityFactoryContext> EntityFactory::makeStandaloneContext()
{
	const auto mask = makeMask(EntitySerialization::Type::Prefab, EntitySerialization::Type::SaveData);
	return std::make_shared<EntityFactoryContext>(world, resources, mask, true);
}

void EntityFactory::updateEntityNode(const IEntityData& iData, EntityRef entity, std::optional<EntityRef> parent, const std::shared_ptr<EntityFactoryContext>& context)
{
	assert(entity.isValid());
	if (parent) {
		entity.setParent(parent.value());
	}

	context->setCurrentEntity(entity);

	if (iData.isDelta()) {
		const auto& delta = iData.asEntityDataDelta();
		if (delta.getName()) {
			entity.setName(delta.getName().value());
		}
		if (delta.getFlags()) {
			entity.setSelectable((delta.getFlags().value() & static_cast<uint8_t>(EntityData::Flag::NotSelectable)) == 0);
		}
		entity.setPrefab(context->getPrefab(), delta.getPrefabUUID().value_or(entity.getPrefabUUID()));
		updateEntityComponentsDelta(entity, delta, *context);
		updateEntityChildrenDelta(entity, delta, context);
	} else {
		const auto& data = iData.asEntityData();
		entity.setName(data.getName());
		entity.setSelectable(!data.getFlag(EntityData::Flag::NotSelectable));
		if (data.getPrefabUUID().isValid()) {
			entity.setPrefab(context->getPrefab(), data.getPrefabUUID());
		}
		updateEntityComponents(entity, data, *context);
		updateEntityChildren(entity, data, context);
	}

	// Don't keep a lingering reference
	context->setCurrentEntity(EntityRef());
}

void EntityFactory::updateEntityComponents(EntityRef entity, const EntityData& data, const EntityFactoryContext& context)
{
	const auto& func = world.getCreateComponentFunction();

	if (entity.getNumComponents() == 0) {
		// Simple population
		for (const auto& [componentName, componentData]: data.getComponents()) {
			try {
				func(context, componentName, entity, componentData);
			} catch (...) {
				Logger::logError("Unable to create component \"" + componentName + "\".");
			}
		}
	} else {
		// Store the existing ids
		Vector<int> existingComps;
		for (auto& c: entity) {
			existingComps.push_back(c.first);
		}

		// Populate
		for (const auto& [componentName, componentData]: data.getComponents()) {
			try {
				const auto result = func(context, componentName, entity, componentData);
				if (!result.created) {
					existingComps.erase(std::find(existingComps.begin(), existingComps.end(), result.componentId));
				}
			} catch (...) {
				Logger::logError("Unable to update component \"" + componentName + "\".");
			}
		}

		// Remove old
		for (auto& id: existingComps) {
			entity.removeComponentById(id);
		}
	}
}

void EntityFactory::updateEntityComponentsDelta(EntityRef entity, const EntityDataDelta& delta, const EntityFactoryContext& context)
{
	const auto& func = world.getCreateComponentFunction();

	for (const auto& [componentName, componentData]: delta.getComponentsChanged()) {
		auto overriden = getComponentsWithPrefabDefaults(entity, context, componentData, componentName);		
		func(context, componentName, entity, overriden ? *overriden : componentData);
	}

	for (const auto& componentName: delta.getComponentsRemoved()) {
		func(context, componentName, entity, ConfigNode(ConfigNode::DelType()));
	}
}

std::optional<ConfigNode> EntityFactory::getComponentsWithPrefabDefaults(EntityRef entity, const EntityFactoryContext& context, const ConfigNode& componentData, const String& componentName)
{
	std::optional<ConfigNode> result;

	for (const auto& entry: componentData.asMap()) {
		if (entry.second.getType() == ConfigNodeType::Del) {
			if (const auto & prefab = context.getPrefab(); prefab) {
				if (const auto * prefabData = prefab->getEntityData().tryGetPrefabUUID(entity.getPrefabUUID()); prefabData) {
					const auto& fieldName = entry.first;

					if (auto& data = prefabData->getFieldData(componentName, fieldName); data.getType() != ConfigNodeType::Undefined) {
						if (!result) {
							result = componentData;
						}
						(*result)[fieldName] = data;
					}
				}
			}
		}
	}

	return result;
}

void EntityFactory::updateEntityChildren(EntityRef entity, const EntityData& data, const std::shared_ptr<EntityFactoryContext>& context)
{
	if (!entity.getRawChildren().empty()) {
		// Delete old children that are no longer present
		const auto& newChildren = data.getChildren();
		Vector<EntityRef> toDelete;
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
			const auto newContext = makeContext(child, entity, context->getScene(), context->isUpdateContext(), context->getEntitySerializationContext().entitySerializationTypeMask, context.get());
			updateEntityNode(newContext->getRootEntityData(), getEntity(child.getInstanceUUID(), *newContext, false), entity, newContext);
		} else {
			updateEntityNode(child, getEntity(child.getInstanceUUID(), *context, false), entity, context);
		}
	}

	// Ensure children order
	Vector<UUID> childInstanceUUIDs;
	childInstanceUUIDs.reserve(data.getChildren().size());
	for (const auto& child: data.getChildren()) {
		childInstanceUUIDs.push_back(child.getInstanceUUID());
	}
	entity.sortChildrenByInstanceUUIDs(childInstanceUUIDs);
}

void EntityFactory::updateEntityChildrenDelta(EntityRef entity, const EntityDataDelta& delta, const std::shared_ptr<EntityFactoryContext>& context)
{
	Vector<EntityRef> toDelete;
	for (auto child: entity.getChildren()) {
		if (std_ex::contains(delta.getChildrenRemoved(), child.getInstanceUUID())) {
			toDelete.emplace_back(child);
		} else {
			const auto iter = std::find_if(delta.getChildrenChanged().begin(), delta.getChildrenChanged().end(), [&] (const auto& e) { return e.first == child.getInstanceUUID() || e.first == child.getPrefabUUID(); });
			if (iter != delta.getChildrenChanged().end()) {
				updateEntityNode(iter->second, child, entity, context);
			}
		}
	}
	for (const auto& childData: delta.getChildrenAdded()) {
		updateEntityNode(childData, tryGetEntity(childData.getInstanceUUID(), *context, false), entity, context);
	}
	for (auto& c: toDelete) {
		world.destroyEntity(c);
	}
}

void EntityFactory::preInstantiateEntities(const IEntityData& iData, EntityFactoryContext& context, int depth)
{
	if (iData.isDelta()) {
		const auto& delta = iData.asEntityDataDelta();
		
		for (const auto& child: delta.getChildrenAdded()) {
			preInstantiateEntities(child, context, depth + 1);
		}
		for (const auto& child: delta.getChildrenChanged()) {
			preInstantiateEntities(child.second, context, depth + 1);
		}
	} else {
		const auto& data = iData.asEntityData();
		const auto entity = instantiateEntity(data, context, context.isUpdateContext() && depth == 0);
		if (depth == 0) {
			context.notifyEntity(entity);
		}
		
		for (const auto& child: data.getChildren()) {
			preInstantiateEntities(child, context, depth + 1);
		}
	}
}

EntityRef EntityFactory::instantiateEntity(const EntityData& data, EntityFactoryContext& context, bool allowWorldLookup)
{
	const auto existing = tryGetEntity(data.getInstanceUUID(), context, allowWorldLookup);
	if (existing.isValid()) {
		return existing;
	}
	
	auto entity = world.createEntity(data.getInstanceUUID(), data.getName(), std::optional<EntityRef>(), context.getWorldPartition());
	if (data.getPrefabUUID().isValid()) {
		entity.setPrefab(context.getPrefab(), data.getPrefabUUID());
	}
	context.addEntity(entity);

	return entity;
}

void EntityFactory::collectExistingEntities(EntityRef entity, EntityFactoryContext& context)
{
	// TODO: this does not collect entities on the same prefab, but above the current entity
	// That case can happen if it's updating an entity whose entity reference is not the root
	// It can be relevant if there are UUIDs pointing upwards, in that situation
	
	context.addEntity(entity);
	entity.setReloaded();
	
	for (const auto c: entity.getChildren()) {
		collectExistingEntities(c, context);
	}
}

EntityRef EntityFactory::tryGetEntity(const UUID& instanceUUID, EntityFactoryContext& context, bool allowWorldLookup)
{
	Expects(instanceUUID.isValid());
	const auto result = context.getEntity(instanceUUID, false, allowWorldLookup);
	if (result.isValid()) {
		return result;
	}

	return EntityRef();
}

EntityRef EntityFactory::getEntity(const UUID& instanceUUID, EntityFactoryContext& context, bool allowWorldLookup)
{
	auto result = tryGetEntity(instanceUUID, context, allowWorldLookup);
	if (!result.isValid()) {
		throw Exception("Unable to find entity with UUID \"" + instanceUUID.toString() + "\"", HalleyExceptions::Entity);
	}
	return result;
}

std::pair<EntityRef, std::optional<UUID>> EntityFactory::loadEntityDelta(const EntityDataDelta& delta, const std::optional<UUID>& uuidSrc)
{
	std::optional<UUID> parentUUID;
	
	const UUID& uuid = uuidSrc.value_or(delta.getInstanceUUID().value_or(UUID::generate()));
	EntityRef entity = uuidSrc ? getWorld().findEntity(uuid, true).value_or(EntityRef()) : EntityRef();
	
	if (entity.isValid() && entity.getPrefabAssetId() == delta.getPrefab()) {
		// Apply delta to existing entity
		updateEntity(entity, delta, static_cast<int>(EntitySerialization::Type::SaveData));
	} else {
		// Generate full EntityData from prefab first
		auto [entityData, prefab, prefabUUID] = prefabDeltaToEntityData(delta);
		
		entityData.setInstanceUUID(uuid);

		if (entity.isValid()) {
			// Update existing entity
			updateEntity(entity, entityData, static_cast<int>(EntitySerialization::Type::SaveData) | static_cast<int>(EntitySerialization::Type::Prefab));
		}  else {
			// Create new entity
			entity = createEntity(entityData);

			// Pending parenting
			if (entityData.getParentUUID().isValid()) {
				parentUUID = entityData.getParentUUID();
			}
		}

		if (prefab) {
			entity.setPrefab(prefab, prefabUUID);
		}
	}

	return std::make_pair(entity, parentUUID);
}

std::tuple<EntityData, std::shared_ptr<const Prefab>, UUID> EntityFactory::prefabDeltaToEntityData(const EntityDataDelta& delta)
{
	if (delta.getPrefab()) {
		auto prefab = resources.get<Prefab>(delta.getPrefab().value());
		const auto& prefabDataRoot = prefab->getEntityData();
		auto prefabUUID = delta.getPrefabUUID().value_or(prefabDataRoot.getPrefabUUID());
		const auto* prefabData = prefabDataRoot.tryGetPrefabUUID(prefabUUID);
		if (!prefabData) {
			throw Exception("Prefab data not found: " + delta.getPrefab().value() + " with UUID " + prefabUUID.toString(), 0);
		}

		auto entityData = EntityData::applyDelta(*prefabData, delta);
		entityData.setPrefab("");

		return { std::move(entityData), std::move(prefab), prefabUUID };
	} else {
		return { EntityData(delta), {}, {} };
	}
}
