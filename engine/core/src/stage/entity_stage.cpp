#include "stage/entity_stage.h"
#include <halley/entity/world.h>
#include <halley/entity/system.h>
#include "halley/file_formats/config_file.h"
using namespace Halley;

std::unique_ptr<World> EntityStage::createWorld(String configName, std::function<std::unique_ptr<System>(String)> createFunction)
{
	auto world = std::make_unique<World>(&getAPI());

	auto config = getResource<ConfigFile>(configName);
	auto& root = config->getRoot();

	auto timelines = root["timelines"].asMap();
	for (auto iter = timelines.begin(); iter != timelines.end(); ++iter) {
		String timelineName = iter->first;
		TimeLine timeline;
		if (timelineName == "fixedUpdate") {
			timeline = TimeLine::FixedUpdate;
		} else if (timelineName == "variableUpdate") {
			timeline = TimeLine::VariableUpdate;
		} else if (timelineName == "render") {
			timeline = TimeLine::Render;
		} else {
			throw Exception("Unknown timeline: " + timelineName);
		}

		for (auto sysName : iter->second) {
			String name = sysName.asString();
			world->addSystem(createFunction(name + "System"), timeline).setName(name);
		}
	}

	return std::move(world);
}
