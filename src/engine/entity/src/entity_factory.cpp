#include "entity_factory.h"
#include "halley/support/logger.h"
#include "world.h"

#define DONT_INCLUDE_HALLEY_HPP
#include "components/transform_2d_component.h"

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

EntityRef EntityFactory::createEntity(const ConfigNode& node)
{
	// Note: this is not thread-safe
	context.entityContext->uuids.clear();

	auto entity = createEntity(nullptr, node, false);
	updateEntityTree(entity, node);
	return entity;
}

EntityRef EntityFactory::createEntity(EntityRef* parent, const ConfigNode& node, bool populate)
{
	const auto uuid = UUID(node["uuid"].asString());
	auto entity = world.createEntity(uuid, node["name"].asString(""));

	context.entityContext->uuids[uuid] = entity.getEntityId();

	updateEntity(entity, node, !populate);

	if (parent) {
		auto parentTransform = parent->tryGetComponent<Transform2DComponent>();
		if (parentTransform) {
			parentTransform->addChild(entity.getComponent<Transform2DComponent>(), true);
		}
	}
	
	if (node["children"].getType() == ConfigNodeType::Sequence) {
		for (auto& childNode: node["children"].asSequence()) {
			createEntity(&entity, childNode, populate);
		}
	}

	return entity;
}

void EntityFactory::updateEntity(EntityRef& entity, const ConfigNode& node, bool transformOnly)
{
	const auto func = world.getCreateComponentFunction();
	if (node["components"].getType() == ConfigNodeType::Sequence) {
		for (auto& componentNode: node["components"].asSequence()) {
			for (auto& c: componentNode.asMap()) {
				auto name = c.first;
				if (!transformOnly || name == "Transform2D" || name == "Transform3D") {
					func(*this, name, entity, c.second);
				}
			}
		}
	}

	entity.setName(node["name"].asString(""));
}

void EntityFactory::updateEntityTree(EntityRef& entity, const ConfigNode& node)
{
	updateEntity(entity, node);

	const auto transform2D = entity.tryGetComponent<Transform2DComponent>();
	if (node["children"].getType() != ConfigNodeType::Sequence || !transform2D) {
		return;
	}

	const auto& childNodes = node["children"].asSequence();
	const size_t nNodes = childNodes.size();

	// Compile the set of all UUIDs that are in the node
	std::vector<UUID> nodeUUIDs;
	nodeUUIDs.reserve(nNodes);
	std::vector<char> nodeConsumed(nNodes, 0);
	for (size_t i = 0; i < nNodes; ++i) {
		nodeUUIDs.push_back(UUID(childNodes[i]["uuid"].asString()));
	}

	// Update the existing children
	size_t childIndex = 0;
	for (auto& child: transform2D->getChildren()) {
		auto childEntity = world.getEntity(child->getEntityId());
		const auto& entityUUID = childEntity.getUUID();

		// Find which node to use based on UUID
		for (size_t i = 0; i < nNodes; ++i) {
			// Start at child index and loop around.
			// If the structure matches (no insertions/removed since creation), this will find it on the first attempt.
			const auto nodeIdx = (i + childIndex) % nNodes;
			if (entityUUID == nodeUUIDs[nodeIdx]) {
				nodeConsumed[nodeIdx] = 1;
				updateEntityTree(childEntity, childNodes[nodeIdx]);
				break;
			}
		}

		childIndex++;
	}

	// Insert new nodes
	for (size_t i = 0; i < nNodes; ++i) {
		if (!nodeConsumed[i]) {
			createEntity(&entity, childNodes[i], true);
		}
	}
}

EntitySerializationContext::EntitySerializationContext(World& world)
	: world(world)
{
}
