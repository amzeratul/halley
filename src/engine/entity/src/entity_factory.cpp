#include "entity_factory.h"
#include "halley/support/logger.h"
#include "world.h"
#include "halley/core/resources/resources.h"

using namespace Halley;

EntityFactory::EntityFactory(World& world, Resources& resources)
	: world(world)
	, resources(resources)
	, context(makeContext())
{
}

EntityFactory::~EntityFactory()
{
}

EntityRef EntityFactory::createEntity(const char* prefabName)
{
	return createEntity(getPrefabNode(prefabName));
}

EntityRef EntityFactory::createEntity(const String& prefabName)
{
	return createEntity(getPrefabNode(prefabName));
}

EntityRef EntityFactory::createEntity(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Sequence) {
		throw Exception("Prefab seems to have more than one root; use EntityFactory::createScene() instead", HalleyExceptions::Entity);
	}
	
	startContext();
	return createEntityTree(node);
}

std::vector<EntityRef> EntityFactory::createScene(const ConfigNode& node)
{
	startContext();
	
	std::vector<EntityRef> result;
	if (node.getType() == ConfigNodeType::Sequence) {
		for (auto& e: node.asSequence()) {
			result.push_back(createEntityTree(e));
		}
	} else {
		result.push_back(createEntityTree(node));
	}
	return result;
}

EntityRef EntityFactory::createEntityTree(const ConfigNode& node)
{
	auto entity = createEntity(std::optional<EntityRef>(), node, false);
	doUpdateEntityTree(entity, node, false);
	return entity;
}

EntityRef EntityFactory::createEntity(std::optional<EntityRef> parent, const ConfigNode& treeNode, bool populate)
{
	const bool isPrefab = treeNode.hasKey("prefab");
	const auto& node = isPrefab ? getPrefabNode(treeNode["prefab"].asString()) : treeNode;
	
	const auto uuid = UUID(treeNode["uuid"].asString()); // Use UUID in parent, not in prefab
	auto entity = world.createEntity(uuid, node["name"].asString(""), parent);

	context.entityContext->uuids[uuid] = entity.getEntityId();

	if (populate) {
		updateEntity(entity, treeNode, UpdateMode::UpdateAll);
	}

	if (node["children"].getType() == ConfigNodeType::Sequence) {
		for (auto& childNode: node["children"].asSequence()) {
			createEntity(entity, childNode, populate);
		}
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
		if ((*overrideNodes)["components"].getType() == ConfigNodeType::Sequence) {
			auto& sequence = (*overrideNodes)["components"].asSequence();
			for (auto& componentNode: sequence) {
				for (auto& [componentName, componentData]: componentNode.asMap()) {
					overrides[componentName] = &componentData;
				}
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
	const auto func = world.getCreateComponentFunction();
	if (node["components"].getType() == ConfigNodeType::Sequence) {
		auto& sequence = node["components"].asSequence();

		if (mode == UpdateMode::UpdateAllDeleteOld) {
			idsUpdated.reserve(sequence.size());
		}
		
		for (auto& componentNode: sequence) {
			for (auto& [componentName, componentData]: componentNode.asMap()) {
				auto result = func(*this, componentName, entity, getComponentData(componentName, componentData));
				
				if (mode == UpdateMode::UpdateAllDeleteOld) {
					idsUpdated.push_back(result.componentId);
				}
			}
		}
	}

	if (mode == UpdateMode::UpdateAllDeleteOld) {
		entity.keepOnlyComponentsWithIds(idsUpdated);
	}

	entity.setName(node["name"].asString(""));
}

void EntityFactory::updateEntityTree(EntityRef& entity, const ConfigNode& node)
{
	startContext();
	doUpdateEntityTree(entity, node, true);
}

void EntityFactory::updateScene(std::vector<EntityRef>& entities, const ConfigNode& node)
{
	startContext();
	if (node.getType() == ConfigNodeType::Sequence) {
		std::map<String, const ConfigNode*> nodes;

		for (auto& n: node.asSequence()) {
			nodes[n["uuid"].asString()] = &n;
		}
		
		for (auto& e: entities) {
			const auto iter = nodes.find(e.getUUID().toString());
			if (iter != nodes.end()) {
				doUpdateEntityTree(e, *iter->second, true);
			}			
		}
	} else {
		if (entities.size() != 1) {
			throw Exception("Expecting only one entity for non-sequence scene", HalleyExceptions::Entity);
		}
		doUpdateEntityTree(entities.at(0), node, true);
	}
}

void EntityFactory::doUpdateEntityTree(EntityRef& entity, const ConfigNode& treeNode, bool refreshing)
{
	updateEntity(entity, treeNode, refreshing ? UpdateMode::UpdateAllDeleteOld : UpdateMode::UpdateAll);

	const bool isPrefab = treeNode.hasKey("prefab");
	const auto& node = isPrefab ? getPrefabNode(treeNode["prefab"].asString()) : treeNode;

	const auto& childNodes = node["children"].getType() == ConfigNodeType::Sequence ? node["children"].asSequence() : ConfigNode::SequenceType();
	const size_t nNodes = childNodes.size();

	// Compile the set of all UUIDs that are in the node
	std::vector<UUID> nodeUUIDs;
	nodeUUIDs.reserve(nNodes);
	std::vector<char> nodeConsumed(nNodes, 0);
	for (size_t i = 0; i < nNodes; ++i) {
		nodeUUIDs.emplace_back(childNodes[i]["uuid"].asString());
	}

	// Update the existing children
	auto& entityChildren = entity.getRawChildren();
	for (size_t childIndex = 0; childIndex < entityChildren.size();) {
		auto childEntity = EntityRef(*entityChildren[childIndex], world);
		const auto& entityUUID = childEntity.getUUID();

		// Find which node to use based on UUID
		bool found = false;
		for (size_t i = 0; i < nNodes; ++i) {
			// Start at child index and loop around.
			// If the structure matches (no insertions/removed since creation), this will find it on the first attempt.
			const auto nodeIdx = (i + childIndex) % nNodes;
			if (entityUUID == nodeUUIDs[nodeIdx]) {
				nodeConsumed[nodeIdx] = 1;
				found = true;
				doUpdateEntityTree(childEntity, childNodes[nodeIdx], refreshing);
				break;
			}
		}

		// If not found, it will be deleted. Note that deleting an entity immediately removes it from the tree, so we only increase childIndex if it's not removed
		if (found) {
			++childIndex;
		} else {
			world.destroyEntity(childEntity.getEntityId());
		}
	}

	// Insert new nodes
	for (size_t i = 0; i < nNodes; ++i) {
		if (!nodeConsumed[i]) {
			createEntity(entity, childNodes[i], true);
		}
	}
}

const ConfigNode& EntityFactory::getPrefabNode(const String& id)
{
	return context.resources->get<Prefab>(id)->getRoot();
}

void EntityFactory::startContext()
{
	// Warning: this makes this whole class not thread safe
	context.entityContext->clear();
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
