#include "animation.h"
#include "sprite_sheet.h"
#include "../material.h"
#include "../material_parameter.h"
#include "../../core/src/api/halley_api.h"
#include "../../core/src/resources/resources.h"
#include <yaml-cpp/yaml.h>

using namespace Halley;

AnimationFrame::AnimationFrame(int frameNumber, String imageName, SpriteSheet& sheet, const std::vector<AnimationDirection>& directions)
{
	const size_t n = directions.size();
	sprites.resize(n);
	for (size_t i = 0; i < n; i++) {
		auto& sprite = sprites[i];
		String spriteName = directions[i].getFrameName(frameNumber, imageName);
		sprite = &sheet.getSprite(spriteName);
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

		if (root["directions"].IsDefined()) {
			for (auto& directionNode : root["directions"]) {
				String name = directionNode["name"].as<std::string>();
				String fileName = directionNode["fileName"].as<std::string>(name);
				bool flip = directionNode["flip"].as<bool>(false);
				size_t idx = directions.size();
				directions.emplace_back(AnimationDirection(name, fileName, flip, idx));
			}
		} else {
			directions.emplace_back(AnimationDirection("default", "default", false, 0));
		}

		for (auto& sequenceNode : root["sequences"]) {
			AnimationSequence sequence;
			sequence.name = sequenceNode["name"].as<std::string>();
			sequence.fps = sequenceNode["fps"].as<float>();

			String fileName = sequenceNode["fileName"].as<std::string>();
			for (auto& frameNode : sequenceNode["frames"]) {
				String value = frameNode.as<std::string>();
				std::vector<int> values;
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
					AnimationFrame frame(number, fileName, *spriteSheet, directions);
					sequence.frames.emplace_back(frame);
				}
			}

			sequences.emplace_back(sequence);
		}
	}
	catch (Exception& e) {
		throw e;
	}
	catch (std::exception& e) {
		throw Exception("Exception parsing animation " + loader.getName() + ": " + e.what());
	}
}
