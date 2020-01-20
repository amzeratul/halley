#include "stage/entity_stage.h"
#include <halley/entity/world.h>
#include <halley/entity/system.h>
#include "halley/file_formats/config_file.h"
#include "game/game.h"
using namespace Halley;

std::unique_ptr<World> EntityStage::createWorld(const String& configName, std::function<std::unique_ptr<System>(String)> createFunction, CreateComponentFunction createComponent)
{
	auto world = std::make_unique<World>(&getAPI(), getGame().isDevMode(), std::move(createComponent));

	auto config = getResource<ConfigFile>(configName);
	world->loadSystems(getResource<ConfigFile>(configName)->getRoot(), std::move(createFunction));

	return world;
}
