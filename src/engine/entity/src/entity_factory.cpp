#include "entity_factory.h"
#include "world.h"
using namespace Halley;

EntityFactory::EntityFactory(World& world, Resources& resources)
	: world(world)
	, resources(resources)
{
}

Maybe<EntityRef> EntityFactory::createEntity(const ConfigNode& node)
{
	if (!node["enabled"].asBool(true)) {
		return {};
	}
	
	auto entity = world.createEntity();
	const auto func = world.getCreateComponentFunction();
	
	for (auto& componentNode: node["components"].asSequence()) {
		for (auto& c: componentNode.asMap()) {
			auto name = c.first;
			func(*this, name, entity, c.second);
		}
	}

	return entity;
}

void EntityFactory::createScene(const ConfigNode& node)
{
	for (auto& e: node["entities"].asSequence()) {
		createEntity(e);
	}
}
