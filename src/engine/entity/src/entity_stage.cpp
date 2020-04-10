#include "halley/file_formats/config_file.h"
#include "halley/core/game/game.h"
#include "world.h"
#include "entity_stage.h"
#include "system.h"
#include "registry.h"
using namespace Halley;

std::unique_ptr<World> EntityStage::createWorld(const String& configName)
{
	auto world = std::make_unique<World>(getAPI(), getResources(), getGame().isDevMode(), createComponent);
	world->loadSystems(getResource<ConfigFile>(configName)->getRoot(), createSystem);

	return world;
}
