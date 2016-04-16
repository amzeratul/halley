#include "animation.h"
#include "sprite_sheet.h"
#include "../material.h"
#include "../material_parameter.h"
#include "../../core/src/api/halley_api.h"
#include "../../core/src/resources/resources.h"
#include <yaml-cpp/yaml.h>

using namespace Halley;

std::unique_ptr<Animation> Animation::loadResource(ResourceLoader& loader)
{
	return std::unique_ptr<Animation>(new Animation(loader));
}

Animation::Animation(ResourceLoader& loader)
{
	YAML::Node root;
	try {
		root = YAML::Load(loader.getStatic()->getString());
	} catch (std::exception& e) {
		throw Exception("Exception parsing animation " + loader.getName() + ": " + e.what());
	}

	String basePath = loader.getBasePath();
	auto& resources = loader.getAPI().core->getResources();

	name = root["name"].as<std::string>();
	
	if (root["spriteSheet"].IsDefined()) {
		String path = basePath + root["spriteSheet"].as<std::string>();
		spriteSheet = resources.get<SpriteSheet>(path);
	}

	if (root["shader"].IsDefined()) {
		String path = basePath + root["shader"].as<std::string>();
		material = resources.get<Material>(path);
		(*material)["tex0"] = spriteSheet->getTexture();
	}
}
