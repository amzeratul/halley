#include "halley/file_formats/config_file.h"
#include "halley/game/game.h"
#include "halley/entity/world.h"
#include "halley/entity/entity_stage.h"
using namespace Halley;

std::unique_ptr<World> EntityStage::createWorld(const String& configName)
{
	return World::make(getAPI(), getResources(), configName, getGame().isDevMode());
}
