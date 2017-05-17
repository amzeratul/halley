#include "stage/entity_stage.h"
#include <halley/entity/world.h>
#include <halley/entity/system.h>
#include "halley/file_formats/config_file.h"
using namespace Halley;

std::unique_ptr<World> EntityStage::createWorld(String configName, std::function<std::unique_ptr<System>(String)> createFunction)
{
	auto world = std::make_unique<World>(&getAPI());

	auto config = getResource<ConfigFile>(configName);
	world->loadSystems(getResource<ConfigFile>(configName)->getRoot(), createFunction);

	return world;
}
