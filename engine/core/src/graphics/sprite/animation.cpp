#include "halley/core/graphics/sprite/animation.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/api/halley_api.h"
#include "resources/resources.h"
#include "halley/file/byte_serializer.h"
#include <gsl/gsl_assert>

using namespace Halley;

AnimationFrame::AnimationFrame(int frameNumber, const String& imageName, const SpriteSheet& sheet, const Vector<AnimationDirection>& directions)
{
	const size_t n = directions.size();
	sprites.resize(n);
	for (size_t i = 0; i < n; i++) {
		sprites[i] = &sheet.getSprite(directions[i].getFrameName(frameNumber, imageName));
	}
}

AnimationFrameDefinition::AnimationFrameDefinition()
	: frameNumber(-1)
{}

AnimationFrameDefinition::AnimationFrameDefinition(int frameNumber, const String& imageName)
	: imageName(imageName)
	, frameNumber(frameNumber)
{
}

AnimationFrame AnimationFrameDefinition::makeFrame(const SpriteSheet& sheet, const Vector<AnimationDirection>& directions) const
{
	return AnimationFrame(frameNumber, imageName, sheet, directions);
}

void AnimationFrameDefinition::serialize(Serializer& s) const
{
	s << imageName;
	s << frameNumber;
}

void AnimationFrameDefinition::deserialize(Deserializer& s)
{
	s >> imageName;
	s >> frameNumber;
}

AnimationSequence::AnimationSequence() {}

AnimationSequence::AnimationSequence(String name, float fps, bool loop, bool noFlip)
	: name(name)
	, fps(fps)
	, loop(loop)
	, noFlip(noFlip)
{}

void AnimationSequence::serialize(Serializer& s) const
{
	s << frameDefinitions;
	s << name;
	s << fps;
	s << loop;
	s << noFlip;
}

void AnimationSequence::deserialize(Deserializer& s)
{
	s >> frameDefinitions;
	s >> name;
	s >> fps;
	s >> loop;
	s >> noFlip;
}

void AnimationSequence::addFrame(const AnimationFrameDefinition& animationFrameDefinition)
{
	frameDefinitions.push_back(animationFrameDefinition);
}

AnimationDirection::AnimationDirection()
	: id(-1)
	, flip(false)
{}

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
		baseName = baseName.replaceAll("%dir%", fileName);

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

void AnimationDirection::serialize(Serializer& s) const
{
	s << name;
	s << fileName;
	s << id;
	s << flip;
}

void AnimationDirection::deserialize(Deserializer& s)
{
	s >> name;
	s >> fileName;
	s >> id;
	s >> flip;
}

Animation::Animation() 
{}

std::unique_ptr<Animation> Animation::loadResource(ResourceLoader& loader)
{
	auto result = std::make_unique<Animation>();
	auto sData = loader.getStatic();
	Deserializer s(sData->getSpan());
	s >> *result;
	result->loadDependencies(loader);
	return result;
}

void Animation::loadDependencies(ResourceLoader& loader)
{
	spriteSheet = loader.getAPI().getResource<SpriteSheet>(spriteSheetName);

	auto matDef = loader.getAPI().getResource<MaterialDefinition>(materialName);
	material = std::make_shared<Material>(matDef);
	material->set("tex0", spriteSheet->getTexture());

	for (auto& s: sequences) {
		for (auto& f : s.frameDefinitions) {
			s.frames.emplace_back(f.makeFrame(*spriteSheet, directions));
		}
	}
}

void Animation::setName(const String& n)
{
	name = n;
}

void Animation::setMaterialName(const String& n)
{
	materialName = n;
}

void Animation::setSpriteSheetName(const String& n)
{
	spriteSheetName = n;
}

void Animation::addSequence(const AnimationSequence& sequence)
{
	sequences.push_back(sequence);
}

void Animation::addDirection(const AnimationDirection& direction)
{
	directions.push_back(direction);
}

const AnimationSequence& Animation::getSequence(const String& name) const
{
	for (auto& seq: sequences) {
		if (seq.name == name) {
			return seq;
		}
	}
	return sequences[0];
}

const AnimationDirection& Animation::getDirection(const String& name) const
{
	Expects(directions.size() > 0);

	for (auto& dir : directions) {
		if (dir.name == name) {
			return dir;
		}
	}
	return directions[0];
}

const AnimationDirection& Animation::getDirection(int id) const
{
	Expects(id >= 0);
	Expects(directions.size() > 0);

	if (id < directions.size()) {
		return directions[id];
	} else {
		return directions[0];
	}
}

bool Animation::hasSequence(const String& name) const
{
	for (auto& s: sequences) {
		if (s.getName() == name) {
			return true;
		}
	}
	return false;
}

void Animation::serialize(Serializer& s) const
{
	s << name;
	s << spriteSheetName;
	s << materialName;
	s << sequences;
	s << directions;
}

void Animation::deserialize(Deserializer& s)
{
	s >> name;
	s >> spriteSheetName;
	s >> materialName;
	s >> sequences;
	s >> directions;
}
