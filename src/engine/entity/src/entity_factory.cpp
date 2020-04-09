#include "entity_factory.h"
#include "halley/support/logger.h"
#include "world.h"
#include "halley/core/resources/resources.h"

using namespace Halley;

EntityFactory::EntityFactory(World& world, Resources& resources)
	: world(world)
{
	entityContext = std::make_unique<EntitySerializationContext>(world);
	context.resources = &resources;
	context.entityContext = entityContext.get();
}

EntityFactory::~EntityFactory()
{
}

EntityRef EntityFactory::createEntity(const char* prefabName)
{
	return createEntity(context.resources->get<Prefab>(prefabName)->getRoot());
}

EntityRef EntityFactory::createEntity(const String& prefabName)
{
	return createEntity(context.resources->get<Prefab>(prefabName)->getRoot());
}

EntityRef EntityFactory::createEntity(const ConfigNode& node)
{
	// Note: this is not thread-safe
	context.entityContext->uuids.clear();

	auto entity = createEntity(std::optional<EntityRef>(), node, false);
	doUpdateEntityTree(entity, node, false);
	return entity;
}

EntityRef EntityFactory::createEntity(std::optional<EntityRef> parent, const ConfigNode& node, bool populate)
{
	const auto uuid = UUID(node["uuid"].asString());
	auto entity = world.createEntity(uuid, node["name"].asString(""), parent);

	context.entityContext->uuids[uuid] = entity.getEntityId();

	if (populate) {
		updateEntity(entity, node, UpdateMode::UpdateAll);
	}

	if (node["children"].getType() == ConfigNodeType::Sequence) {
		for (auto& childNode: node["children"].asSequence()) {
			createEntity(entity, childNode, populate);
		}
	}

	return entity;
}

void EntityFactory::updateEntity(EntityRef& entity, const ConfigNode& node, UpdateMode mode)
{
	std::vector<int> idsUpdated;
	
	const auto func = world.getCreateComponentFunction();
	if (node["components"].getType() == ConfigNodeType::Sequence) {
		auto& sequence = node["components"].asSequence();

		if (mode == UpdateMode::UpdateAllDeleteOld) {
			idsUpdated.reserve(sequence.size());
		}
		
		for (auto& componentNode: sequence) {
			for (auto& c: componentNode.asMap()) {
				auto name = c.first;
				auto result = func(*this, name, entity, c.second);
				
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
	doUpdateEntityTree(entity, node, true);
}

void EntityFactory::doUpdateEntityTree(EntityRef& entity, const ConfigNode& node, bool refreshing)
{
	updateEntity(entity, node, refreshing ? UpdateMode::UpdateAllDeleteOld : UpdateMode::UpdateAll);

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
	for (size_t childIndex = 0; childIndex < entityChildren.size(); ++childIndex) {
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

		// Not found, so it should be removed
		if (!found) {
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

EntitySerializationContext::EntitySerializationContext(World& world)
	: world(world)
{
}
