#include "entity_factory.h"
#include "world.h"
using namespace Halley;

EntityFactory::EntityFactory(World& world, Resources& resources)
	: world(world)
	, resources(resources)
{
}

EntityFactory::~EntityFactory()
{
}

EntityEntry EntityFactory::createEntity(const ConfigNode& node)
{
	auto entity = world.createEntity();
	const auto func = world.getCreateComponentFunction();
	
	for (auto& componentNode: node["components"].asSequence()) {
		for (auto& c: componentNode.asMap()) {
			auto name = c.first;
			func(*this, name, entity, c.second);
		}
	}

	return EntityEntry{ node["name"].asString(""), entity };
}

std::vector<EntityEntry> EntityFactory::createScene(const ConfigNode& node)
{
	const auto& seq = node["entities"].asSequence();
	std::vector<EntityEntry> result;
	result.reserve(seq.size());
	
	for (auto& e: seq) {
		result.push_back(createEntity(e));
	}

	return result;
}
