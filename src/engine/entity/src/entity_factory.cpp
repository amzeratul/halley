#include "entity_factory.h"
#include "world.h"
using namespace Halley;

EntityFactory::EntityFactory(World& world)
	: world(world)
{	
}

EntityRef EntityFactory::createEntity(const ConfigNode& node)
{
	auto entity = world.createEntity();
	const auto func = world.getCreateComponentFunction();
	
	for (auto& componentNode: node["components"].asSequence()) {
		auto name = componentNode["name"].asString();
		func(name, entity, componentNode["members"]);
	}

	return entity;
}
