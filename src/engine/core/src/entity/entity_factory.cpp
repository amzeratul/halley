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
	auto entity = world.createEntity();
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

	if (node["children"].getType() == ConfigNodeType::Sequence) {
		auto transform2D = entity.tryGetComponent<Transform2DComponent>();
		if (transform2D) {
			// TODO: match by ID
			const auto& childNodes = node["children"].asSequence();
			size_t i = 0;
			for (auto& child: transform2D->getChildren()) {
				auto childEntity = world.getEntity(child);
				updateEntityTree(childEntity, childNodes[i++]);
			}
		}
	}
}
