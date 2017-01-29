#include "animation_importer.h"
#include <yaml-cpp/yaml.h>
#include "halley/core/graphics/sprite/animation.h"
#include "halley/support/exception.h"
#include "halley/file/byte_serializer.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

void AnimationImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	Animation animation;
	parseAnimation(animation, gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));

	Path dst = asset.inputFiles[0].name.replaceExtension("");
	collector.output(dst, Serializer::toBytes(animation));
}

void AnimationImporter::parseAnimation(Animation& animation, gsl::span<const gsl::byte> data)
{
	String strData(reinterpret_cast<const char*>(data.data()), data.size());
	YAML::Node root = YAML::Load(strData.cppStr());

	animation.setName(root["name"].as<std::string>());

	if (root["spriteSheet"].IsDefined()) {
		animation.setSpriteSheetName(root["spriteSheet"].as<std::string>());
	}

	if (root["material"].IsDefined()) {
		animation.setMaterialName(root["material"].as<std::string>());
	}

	size_t nDirections = 0;
	if (root["directions"].IsDefined()) {
		for (auto directionNode : root["directions"]) {
			String name = directionNode["name"].as<std::string>("default");
			String fileName = directionNode["fileName"].as<std::string>(name);
			bool flip = directionNode["flip"].as<bool>(false);
			animation.addDirection(AnimationDirection(name, fileName, flip, int(nDirections)));
			nDirections++;
		}
	} else {
		animation.addDirection(AnimationDirection("default", "default", false, 0));
	}

	for (auto sequenceNode : root["sequences"]) {
		String name = sequenceNode["name"].as<std::string>("default");
		float fps = sequenceNode["fps"].as<float>(0.0f);
		bool loop = sequenceNode["loop"].as<bool>(true);
		bool noFlip = sequenceNode["noFlip"].as<bool>(false);
		AnimationSequence sequence(name, fps, loop, noFlip);
		String fileName = sequenceNode["fileName"].as<std::string>();

		// Load frames
		size_t framesAdded = 0;
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
					sequence.addFrame(AnimationFrameDefinition(number, fileName));
					framesAdded++;
				}
			}
		}

		// No frames listed, 
		if (framesAdded == 0) {
			sequence.addFrame(AnimationFrameDefinition(0, fileName));
		}

		animation.addSequence(sequence);
	}
}
