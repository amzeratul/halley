#include "graphics/sprite/animation.h"
#include "graphics/sprite/sprite_sheet.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/api/halley_api.h"
#include "resources/resources.h"
#include <yaml-cpp/yaml.h>

using namespace Halley;

AnimationFrame::AnimationFrame(int frameNumber, String imageName, SpriteSheet& sheet, const Vector<AnimationDirection>& directions)
{
	const size_t n = directions.size();
	sprites.resize(n);
	for (size_t i = 0; i < n; i++) {
		sprites[i] = &sheet.getSprite(directions[i].getFrameName(frameNumber, imageName));
	}
}

AnimationDirection::AnimationDirection(String name, String fileName, bool flip, int id)
	: name(name)
	, fileName(fileName)
	, id(id)
	, flip(flip)
{
}

String AnimationDirection::getFrameName(int frameNumber, String baseName) const
{
	try {
		baseName.replace("%dir%", fileName);

		size_t frameToken = baseName.find("%f");
		if (frameToken != std::string::npos) {
			size_t endToken = baseName.find('%', frameToken + 1);
			if (endToken != std::string::npos) {
				int minWidth = 0;
				if (baseName[frameToken + 2] == ':' && baseName[frameToken + 3] == '0') {
					minWidth = baseName.subToInteger(frameToken + 4, endToken);
				}

				std::stringstream ss;
				ss << std::setfill('0') << std::setw(minWidth) << frameNumber;
				baseName = baseName.left(frameToken) + ss.str() + baseName.mid(endToken + 1);
			}
		}

		return baseName;
	} catch (...) {
		throw Exception("Invalid frame name: " + baseName);
	}
}

std::unique_ptr<Animation> Animation::loadResource(ResourceLoader& loader)
{
	return std::unique_ptr<Animation>(new Animation(loader));
}

const AnimationSequence& Animation::getSequence(String name) const
{
	for (auto& seq: sequences) {
		if (seq.name == name) {
			return seq;
		}
	}
	return sequences[0];
}

const AnimationDirection& Animation::getDirection(String name) const
{
	for (auto& dir : directions) {
		if (dir.name == name) {
			return dir;
		}
	}
	return directions[0];
}

Animation::Animation(ResourceLoader& loader)
{
	try {
		YAML::Node root = YAML::Load(loader.getStatic()->getString());

		String basePath = loader.getBasePath();

		name = root["name"].as<std::string>();

		if (root["spriteSheet"].IsDefined()) {
			String path = root["spriteSheet"].as<std::string>();
			spriteSheet = loader.getAPI().getResource<SpriteSheet>(path);
		}

		if (root["shader"].IsDefined()) {
			String path = root["shader"].as<std::string>();
			auto matDef = loader.getAPI().getResource<MaterialDefinition>(path);
			material = std::make_shared<Material>(matDef);
			(*material)["tex0"] = spriteSheet->getTexture();
		}

		if (root["directions"].IsDefined()) {
			for (auto directionNode : root["directions"]) {
				String name = directionNode["name"].as<std::string>("default");
				String fileName = directionNode["fileName"].as<std::string>(name);
				bool flip = directionNode["flip"].as<bool>(false);
				size_t idx = directions.size();
				directions.emplace_back(AnimationDirection(name, fileName, flip, int(idx)));
			}
		} else {
			directions.emplace_back(AnimationDirection("default", "default", false, 0));
		}

		for (auto sequenceNode : root["sequences"]) {
			AnimationSequence sequence;
			sequence.name = sequenceNode["name"].as<std::string>("default");
			sequence.fps = sequenceNode["fps"].as<float>(0.0f);
			sequence.loop = sequenceNode["loop"].as<bool>(true);
			sequence.noFlip = sequenceNode["noFlip"].as<bool>(false);
			String fileName = sequenceNode["fileName"].as<std::string>();

			// Load frames
			if (sequenceNode["frames"].IsDefined()) {
				for (auto frameNode : sequenceNode["frames"]) {
					String value = frameNode.as<std::string>();
					Vector<int> values;
					if (value.isInteger()) {
						values.push_back(value.toInteger());
					} else if (value.contains("-")) {
						auto split = value.split('-');
						if (split.size() == 2 && split[0].isInteger() && split[1].isInteger()) {
							int a = split[0].toInteger();
							int b = split[1].toInteger();
							int dir = a < b ? 1 : -1;
							for (int i = a; i != b + dir; i += dir) {
								values.push_back(i);
							}
						} else {
							throw Exception("Invalid frame token: " + value);
						}
					}

					for (int number : values) {
						sequence.frames.emplace_back(AnimationFrame(number, fileName, *spriteSheet, directions));
					}
				}
			}

			// No frames listed, 
			if (sequence.frames.size() == 0) {
				sequence.frames.emplace_back(AnimationFrame(0, fileName, *spriteSheet, directions));
			}

			sequences.emplace_back(sequence);
		}
	}
	catch (std::exception& e) {
		throw Exception("Exception parsing animation " + loader.getName() + ": " + e.what());
	}
}
