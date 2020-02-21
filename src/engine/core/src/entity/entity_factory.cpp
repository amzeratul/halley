#include "entity/entity_factory.h"
#include "halley/halley_entity.h"

#define DONT_INCLUDE_HALLEY_HPP
#include "entity/components/transform_2d_component.h"
#include "halley/support/logger.h"

using namespace Halley;

EntityFactory::EntityFactory(World& world, Resources& resources)
	: world(world)
	, resources(resources)
{
}

EntityFactory::~EntityFactory()
{
}

EntityRef EntityFactory::createEntity(const ConfigNode& node, bool populate)
{
	auto entity = world.createEntity(UUID(node["uuid"].asString()), node["name"].asString(""));
	const auto func = world.getCreateComponentFunction();

	if (populate) {
		updateEntity(entity, node);
	}
	
	if (node["children"].getType() == ConfigNodeType::Sequence) {
		for (auto& childNode: node["children"].asSequence()) {
			createChildEntity(entity, childNode, populate);
		}
	}

	return entity;
}

void EntityFactory::createChildEntity(EntityRef& parent, const ConfigNode& node, bool populate)
{
	auto e = createEntity(node, populate);

	auto parentTransform = parent.tryGetComponent<Transform2DComponent>();
	if (parentTransform) {
		parentTransform->addChild(e.getComponent<Transform2DComponent>(), true);
	}
}

void EntityFactory::updateEntity(EntityRef& entity, const ConfigNode& node)
{
	const auto func = world.getCreateComponentFunction();
	if (node["components"].getType() == ConfigNodeType::Sequence) {
		for (auto& componentNode: node["components"].asSequence()) {
			for (auto& c: componentNode.asMap()) {
				auto name = c.first;
				func(*this, name, entity, c.second);
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
		auto childEntity = world.getEntity(child);
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
			createChildEntity(entity, childNodes[i], true);
		}
	}
}
