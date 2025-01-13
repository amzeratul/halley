#include "halley/file_formats/config_file.h"
#include "halley/game/game.h"
#include "halley/entity/world.h"
#include "halley/entity/entity_stage.h"

#include "halley/entity/inspector.h"
using namespace Halley;

void EntityStage::init()
{
	Stage::init();

	if (auto* devcon = getCoreAPI().getDevConClient()) {
		inspectorClient = std::make_shared<InspectorClient>(*devcon);
	}
}

void EntityStage::onVariableUpdate(Time dt)
{
	Stage::onVariableUpdate(dt);

	if (inspectorClient) {
		inspectorClient->update(getWorlds().span());
	}
}

Vector<World*> EntityStage::getWorlds()
{
	return {};
}

std::unique_ptr<World> EntityStage::createWorld(const String& configName, const std::optional<String>& systemTag)
{
	return World::make(getAPI(), getResources(), configName, systemTag, getGame().isDevMode());
}
