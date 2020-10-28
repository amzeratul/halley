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
	return createPrefab(getPrefab(prefabName));
}

EntityRef EntityFactory::createEntity(const String& prefabName)
{
	return createPrefab(getPrefab(prefabName));
}

EntityRef EntityFactory::createEntity(const ConfigNode& node, EntitySerialization::Type sourceType)
{
	startContext(sourceType);
	return createEntityTree(node, nullptr, false, false);
}

EntityRef EntityFactory::createPrefab(std::shared_ptr<const Prefab> prefab)
{
	if (!prefab) {
		Logger::logWarning("Missing prefab");
		return EntityRef();
	} else {
		const auto& node = prefab->getRoot();
		if (node.getType() == ConfigNodeType::Sequence) {
			throw Exception("Prefab seems to have more than one root; use EntityFactory::createScene() instead", HalleyExceptions::Entity);
		}
		
		startContext(EntitySerialization::Type::Prefab);
		return createEntityTree(node, nullptr, true, true);
	}
}

EntityScene EntityFactory::createScene(std::shared_ptr<const Prefab> prefab)
{
	startContext(EntitySerialization::Type::Prefab);
	
	EntityScene scene;	
	const auto& node = prefab->getRoot();
	if (node.getType() == ConfigNodeType::Sequence) {
		int i = 0;
		for (auto& e: node.asSequence()) {
			createEntityTreeForScene(e, scene, prefab, i++);
		}
	} else {
		createEntityTreeForScene(node, scene, prefab);
	}
	return scene;
}

void EntityFactory::createEntityTreeForScene(const ConfigNode& node, EntityScene& curScene, std::shared_ptr<const Prefab> prefab, std::optional<int> index)
{
	auto entity = createEntityTree(node, &curScene, false, false);
	curScene.addPrefabReference(prefab, entity, index);
	curScene.addRootEntity(entity);
}

EntityRef EntityFactory::createEntityTree(const ConfigNode& node, EntityScene* scene, bool fromPrefab, bool fromNewPrefab)
{
	auto entity = createEntity(std::optional<EntityRef>(), std::optional<EntityRef>(), node, false, scene, fromPrefab, true, fromNewPrefab);
	doUpdateEntityTree(entity, node, false, fromPrefab);
	return entity;
}

void EntityFactory::rebuildPrefabContext(const ConfigNode& treeNode, bool isRoot)
{
	if (!treeNode.hasKey("prefabUUID")) {
		return;
	}

	if (!isRoot && treeNode.hasKey("prefab")) {
		return;
	}
	
	auto& currentMapping = context.entityContext->uuidMapping.back();
	const auto& instanceUUID = UUID(treeNode["uuid"].asBytes());
	const auto& prefabUUID = UUID(treeNode["prefabUUID"].asBytes());
	
	currentMapping[prefabUUID] = instanceUUID;
	if (treeNode["children"].getType() == ConfigNodeType::Sequence) {
		for (const auto& childNode : treeNode["children"].asSequence()) {
			rebuildPrefabContext(childNode, false);
		}
	}
}

void EntityFactory::rebuildPrefabContext(EntityRef& entity)
{
	auto& currentMapping = context.entityContext->uuidMapping.back();
	if (currentMapping.find(entity.getPrefabUUID()) != currentMapping.end()) {
		return;
	}
	
	currentMapping[entity.getPrefabUUID()] = entity.getInstanceUUID();
	for (auto child : entity.getChildren()) {
		rebuildPrefabContext(child);
	}
}

void EntityFactory::rebuildContext(EntityRef& entity, const ConfigNode& node)
{
	const auto uuid = getUUID(node["uuid"]); // Use UUID in parent, not in prefab
	context.entityContext->uuids[uuid] = entity.getEntityId();	
	// context.entityContext->uuidMapping.back()[entity.getPrefabUUID()] = uuid;
	if (node["children"].getType() == ConfigNodeType::Sequence) {
		for (const auto& childNode : node["children"].asSequence()) {
			auto child = world.findEntity(UUID(childNode["uuid"].asBytes()), true);
			if (child) {
				rebuildContext(*child, childNode);
			}
		}
	}
}

EntityRef EntityFactory::createEntity(std::optional<EntityRef> parent, std::optional<EntityRef> prefabRoot, const ConfigNode& treeNode, bool populate, EntityScene* curScene, bool fromPrefab, bool isPrefabRoot, bool fromNewPrefab)
{
	const bool isPrefab = treeNode.hasKey("prefab");
	const auto prefab = isPrefab ? getPrefab(treeNode["prefab"].asString()) : std::shared_ptr<const Prefab>();
	const auto& node = isPrefab ? (prefab ? prefab->getRoot() : dummyPrefab) : treeNode;

	if (isPrefab || isPrefabRoot) {
		context.entityContext->uuidMapping.push_back({});
		rebuildPrefabContext(treeNode);
	}
	
	UUID uuid;
	if (prefabRoot && fromPrefab && !isPrefabRoot) {
		const auto& nodeUUID = getUUID(treeNode["uuid"]);
		if (context.entityContext->uuidMapping.back().find(nodeUUID) != context.entityContext->uuidMapping.back().end()) {
			uuid = context.entityContext->uuidMapping.back()[nodeUUID];
		}
		else {
			uuid = UUID::generateFromUUIDs(prefabRoot->getInstanceUUID(), getUUID(treeNode["uuid"]));
		}
	}
	else if (!fromNewPrefab) {
		uuid = getUUID(treeNode["uuid"]);
	}

	auto entity = world.createEntity(uuid, node["name"].asString(""), parent, true, getUUID(node["uuid"]));
	
	if (curScene && prefab) {
		curScene->addPrefabReference(prefab, entity);
	}
	
	context.entityContext->uuids[entity.getInstanceUUID()] = entity.getEntityId();
	context.entityContext->uuidMapping.back()[entity.getPrefabUUID()] = entity.getInstanceUUID();
	
	if (populate) {
		updateEntity(entity, treeNode, UpdateMode::UpdateAll);
	}

	if (node["children"].getType() == ConfigNodeType::Sequence) {
		auto& pr = prefabRoot ? prefabRoot.value() : entity;
		for (auto& childNode: node["children"].asSequence()) {
			createEntity(entity, pr, childNode, populate, curScene, fromPrefab || isPrefab, false, fromNewPrefab);
		}
	}

	if (isPrefab || isPrefabRoot) {
		context.entityContext->uuidMapping.pop_back();
	}
	
	return entity;
}

void EntityFactory::updateEntity(EntityRef& entity, const ConfigNode& treeNode, UpdateMode mode)
{
	const bool isPrefab = treeNode.hasKey("prefab");
	const auto& node = isPrefab ? getPrefabNode(treeNode["prefab"].asString()) : treeNode;
	auto* overrideNodes = isPrefab ? &treeNode : nullptr;

	std::vector<int> idsUpdated;
	
	// Prepare component overrides
	std::map<String, const ConfigNode*> overrides;
	if (overrideNodes) {
		const auto& overrideComps = (*overrideNodes)["components"];
		if (overrideComps.getType() == ConfigNodeType::Sequence) {
			const auto& sequence = overrideComps.asSequence();
			for (const auto& componentNode: sequence) {
				for (const auto& [componentName, componentData]: componentNode.asMap()) {
					overrides[componentName] = &componentData;
				}
			}
		} else if (overrideComps.getType() == ConfigNodeType::Map) {
			for (const auto& [componentName, componentData]: overrideComps.asMap()) {
				overrides[componentName] = &componentData;
			}
		}
	}
	ConfigNode tempNode;
	auto getComponentData = [&] (const String& name, const ConfigNode& originalData) -> const ConfigNode&
	{
		if (isPrefab) {
			const auto iter = overrides.find(name);
			if (iter == overrides.end()) {
				return originalData;
			}
			tempNode = ConfigNode(originalData);
			for (auto& [field, data]: iter->second->asMap()) {
				tempNode[field] = ConfigNode(data);
			}
			return tempNode;
		} else {
			return originalData;
		}
	};

	// Load components
	auto loadComponents = [&] (const ConfigNode::MapType map)
	{
		const auto func = world.getCreateComponentFunction();
		for (const auto& [componentName, componentData]: map) {
			auto result = func(*this, componentName, entity, getComponentData(componentName, componentData));
			
			if (mode == UpdateMode::UpdateAllDeleteOld) {
				idsUpdated.push_back(result.componentId);
			}
		}
	};
	
	if (node["components"].getType() == ConfigNodeType::Sequence) {
		const auto& sequence = node["components"].asSequence();

		if (mode == UpdateMode::UpdateAllDeleteOld) {
			idsUpdated.reserve(sequence.size());
		}
		
		for (const auto& componentNode: sequence) {
			loadComponents(componentNode.asMap());
		}
	} else if (node["components"].getType() == ConfigNodeType::Map) {
		const auto& map = node["components"].asMap();
		
		if (mode == UpdateMode::UpdateAllDeleteOld) {
			idsUpdated.reserve(map.size());
		}

		loadComponents(map);
	}

	if (mode == UpdateMode::UpdateAllDeleteOld) {
		entity.keepOnlyComponentsWithIds(idsUpdated);
	}

	entity.setName(node["name"].asString(""));

	if (mode == UpdateMode::UpdateAllDeleteOld) {
		entity.setReloaded();
	}
}

void EntityFactory::updateEntityTree(EntityRef& entity, const ConfigNode& node, EntitySerialization::Type sourceType, bool doRebuildContext)
{
	startContext(sourceType);
	if (doRebuildContext) {
		rebuildContext(entity, node);
	}
	doUpdateEntityTree(entity, node, true, true);
}

void EntityFactory::updateScene(std::vector<EntityRef>& entities, const ConfigNode& node, EntitySerialization::Type sourceType)
{
	startContext(sourceType);
	if (node.getType() == ConfigNodeType::Sequence) {
		std::map<String, const ConfigNode*> nodes;

		for (auto& n: node.asSequence()) {
			nodes[getUUID(n["uuid"]).toString()] = &n;
		}
		
		for (auto& e: entities) {
			const auto iter = nodes.find(e.getInstanceUUID().toString());
			if (iter != nodes.end()) {
				doUpdateEntityTree(e, *iter->second, true, false);
			}
		}
	} else {
		if (entities.size() != 1) {
			throw Exception("Expecting only one entity for non-sequence scene", HalleyExceptions::Entity);
		}
		doUpdateEntityTree(entities.at(0), node, true, false);
	}
}

ConfigNode EntityFactory::serializeEntity(EntityRef entity, EntitySerialization::Type type)
{
	ConfigNode result = ConfigNode::MapType();
	ConfigNodeSerializationContext serializeContext = makeContext();
	serializeContext.entitySerializationTypeMask = makeMask(type);

	auto components = ConfigNode::SequenceType();
	for (auto [componentId, component]: entity) {
		auto& reflector = getComponentReflector(componentId);
		auto entry = ConfigNode::MapType();
		entry[reflector.getName()] = reflector.serialize(serializeContext, *component);
		components.emplace_back(std::move(entry));
	}

	result["name"] = entity.getName();
	result["uuid"] = entity.getInstanceUUID().getBytes();
	result["prefabUUID"] = entity.getPrefabUUID().getBytes();
	result["components"] = std::move(components);

	auto parent = entity.tryGetParent();
	if (parent) {
		result["parent"] = parent->getInstanceUUID().getBytes();
	}
	
	return result;
}

void EntityFactory::doUpdateEntityTree(EntityRef& entity, const ConfigNode& treeNode, bool refreshing, bool isPrefabRoot)
{
	const bool isPrefab = treeNode.hasKey("prefab");
	const auto& node = isPrefab ? getPrefabNode(treeNode["prefab"].asString()) : treeNode;

	if (isPrefab || isPrefabRoot) {
		context.entityContext->uuidMapping.push_back({});
		rebuildPrefabContext(entity);
		rebuildPrefabContext(treeNode);
	}

	const auto& childNodes = node["children"].getType() == ConfigNodeType::Sequence ? node["children"].asSequence() : ConfigNode::SequenceType();
	const size_t nNodes = childNodes.size();

	// Compile the set of all UUIDs that are in the node
	std::vector<UUID> nodeUUIDs;
	nodeUUIDs.reserve(nNodes);
	std::vector<char> nodeConsumed(nNodes, 0);
	for (size_t i = 0; i < nNodes; ++i) {
		if (childNodes[i].hasKey("prefabUUID")) {
			nodeUUIDs.emplace_back(getUUID(childNodes[i]["prefabUUID"]).toString());
		}
		else {
			nodeUUIDs.emplace_back(getUUID(childNodes[i]["uuid"]).toString());
		}
	}

	// Update the existing children
	auto& entityChildren = entity.getRawChildren();
	for (size_t childIndex = 0; childIndex < entityChildren.size();) {
		auto childEntity = EntityRef(*entityChildren[childIndex], world);
		const auto& entityUUID = childEntity.getPrefabUUID();

		// Find which node to use based on UUID
		bool found = false;
		for (size_t i = 0; i < nNodes; ++i) {
			// Start at child index and loop around.
			// If the structure matches (no insertions/removed since creation), this will find it on the first attempt.
			const auto nodeIdx = (i + childIndex) % nNodes;
			if (entityUUID == nodeUUIDs[nodeIdx]) {
				nodeConsumed[nodeIdx] = 1;
				found = true;
				doUpdateEntityTree(childEntity, childNodes[nodeIdx], refreshing, false);
				break;
			}
		}

		// If not found, and it was originally loaded from prefab, it will be deleted.
		// Note that deleting an entity immediately removes it from the tree, so we only increase childIndex if it's not removed
		if (!found && childEntity.isFromPrefab()) {
			world.destroyEntity(childEntity.getEntityId());
		} else {
			++childIndex;
		}
	}

	// Insert new nodes
	for (size_t i = 0; i < nNodes; ++i) {
		if (!nodeConsumed[i]) {
			createEntity(entity, entity, childNodes[i], true, nullptr, isPrefab, false, false);
		}
	}

	if (isPrefab) {
		if (treeNode["children"].getType() == ConfigNodeType::Sequence) {
			for (auto& childNode : treeNode["children"].asSequence()) {
				auto childEntity = world.findEntity(getUUID(childNode["uuid"]), true);
				if (childEntity) {
					doUpdateEntityTree(childEntity.value(), childNode, refreshing, false);
				}
			}
		}
	}

	entity.sortChildrenByPrefabUUIDs(nodeUUIDs);

	updateEntity(entity, treeNode, refreshing ? UpdateMode::UpdateAllDeleteOld : UpdateMode::UpdateAll);
	
	if (isPrefab || isPrefabRoot) {
		context.entityContext->uuidMapping.pop_back();
	}
}

std::shared_ptr<const Prefab> EntityFactory::getPrefab(const String& id) const
{
	return resources.exists<Prefab>(id) ? resources.get<Prefab>(id) : std::shared_ptr<const Prefab>();
}

const ConfigNode& EntityFactory::getPrefabNode(const String& id) const
{
	const auto p = getPrefab(id);
	if (p) {
		return p->getRoot();
	} else {
		return dummyPrefab;
	}
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

UUID EntityFactory::getUUID(const ConfigNode& node) const
{
	return node.getType() == ConfigNodeType::String ? UUID(node.asString()) : UUID(node.asBytes());
}

EntitySerializationContext::EntitySerializationContext(World& world)
	: world(world)
{
}

void EntitySerializationContext::clear()
{
	uuids.clear();
}
