#include "stage/entity_stage.h"
#include <yaml-cpp/yaml.h>
using namespace Halley;

std::unique_ptr<World> EntityStage::createWorld(String configName, std::function<std::unique_ptr<System>(String)> createFunction)
{
	auto world = std::make_unique<World>(&getAPI());
	auto config = getResource<YAMLFile>("config/" + configName);
	auto& root = config->getRoot();

	auto timelines = root["timelines"];
	for (auto iter = timelines.begin(); iter != timelines.end(); ++iter) {
		String timelineName = iter->first.as<std::string>();
		TimeLine timeline;
		if (timelineName == "fixedUpdate") {
			timeline = TimeLine::FixedUpdate;
		} else if (timelineName == "VariableUpdate") {
			timeline = TimeLine::VariableUpdate;
		} else if (timelineName == "render") {
			timeline = TimeLine::Render;
		} else {
			throw Exception("Unknown timeline: " + timelineName);
		}

		for (auto sysName : iter->second) {
			String name = sysName.as<std::string>();
			world->addSystem(createFunction(name + "System"), timeline).setName(name);
		}
	}

	return std::move(world);
}
