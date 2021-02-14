#include "halley/core/graphics/sprite/animation.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/api/halley_api.h"
#include "resources/resources.h"
#include "halley/bytes/byte_serializer.h"
#include <gsl/gsl_assert>
#include <utility>

using namespace Halley;

AnimationFrame::AnimationFrame(int frameNumber, int duration, const String& imageName, const SpriteSheet& sheet, const Vector<AnimationDirection>& directions)
	: duration(duration)
{
	const size_t n = directions.size();
	sprites.resize(n);
	for (size_t i = 0; i < n; i++) {
		sprites[i] = &sheet.getSprite(directions[i].needsToProcessFrameName(imageName) ? directions[i].getFrameName(frameNumber, imageName) : imageName);
	}
}

int AnimationFrame::getDuration() const
{
	return duration;
}

AnimationFrameDefinition::AnimationFrameDefinition()
	: frameNumber(-1)
{}

AnimationFrameDefinition::AnimationFrameDefinition(int frameNumber, int duration, String imageName)
	: imageName(std::move(imageName))
	, frameNumber(frameNumber)
	, duration(duration)
{
}

AnimationFrame AnimationFrameDefinition::makeFrame(const SpriteSheet& sheet, const Vector<AnimationDirection>& directions) const
{
	return AnimationFrame(frameNumber, duration, imageName, sheet, directions);
}

void AnimationFrameDefinition::serialize(Serializer& s) const
{
	s << imageName;
	s << frameNumber;
	s << duration;
}

void AnimationFrameDefinition::deserialize(Deserializer& s)
{
	s >> imageName;
	s >> frameNumber;
	s >> duration;
}

AnimationSequence::AnimationSequence() {}

AnimationSequence::AnimationSequence(String name, bool loop, bool noFlip)
	: name(std::move(name))
	, loop(loop)
	, noFlip(noFlip)
{}

void AnimationSequence::serialize(Serializer& s) const
{
	s << frameDefinitions;
	s << name;
	s << loop;
	s << noFlip;
}

void AnimationSequence::deserialize(Deserializer& s)
{
	s >> frameDefinitions;
	s >> name;
	s >> loop;
	s >> noFlip;
}

void AnimationSequence::addFrame(const AnimationFrameDefinition& animationFrameDefinition)
{
	frameDefinitions.push_back(animationFrameDefinition);
}

Rect4i AnimationSequence::getBounds() const
{
	Vector2i topLeft(99999, 99999);
	Vector2i bottomRight;
	for (auto& frame: frames) {
		auto& sprite = frame.getSprite(0);
		if (sprite.size.x >= 0.1f && sprite.size.y > 0.0f) {
			auto offset = Vector2i(Vector2f(sprite.pivot * sprite.size).round());
			const Vector2i tl = -offset;
			const Vector2i br = tl + Vector2i(sprite.size);

			topLeft = Vector2i::min(topLeft, tl);
			bottomRight = Vector2i::max(bottomRight, br);
		}
	}

	return Rect4i(topLeft, bottomRight);
}

AnimationDirection::AnimationDirection()
	: id(-1)
	, flip(false)
{}

AnimationDirection::AnimationDirection(String name, String fileName, bool flip, int id)
	: name(std::move(name))
	, fileName(std::move(fileName))
	, id(id)
	, flip(flip)
{
}

bool AnimationDirection::needsToProcessFrameName(const String& baseName) const
{
	return baseName.find("%f") != std::string::npos || baseName.find("%dir%") != std::string::npos;
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
		throw Exception("Invalid frame name: " + baseName, HalleyExceptions::Graphics);
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
	auto sData = loader.getStatic(false);
	if (!sData) {
		return {};
	}
	
	auto result = std::make_unique<Animation>();
	Deserializer s(sData->getSpan());
	s >> *result;
	result->loadDependencies(loader);
	return result;
}

void Animation::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<Animation&>(resource));
}

void Animation::loadDependencies(ResourceLoader& loader)
{
	spriteSheet = loader.getResources().get<SpriteSheet>(spriteSheetName);

	auto matDef = loader.getResources().get<MaterialDefinition>(materialName);
	material = MaterialHandle(matDef);
	material.set("tex0", spriteSheet->getTexture());

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

const AnimationSequence& Animation::getSequence(const String& seqName) const
{
	for (auto& seq: sequences) {
		if (seq.name == seqName) {
			return seq;
		}
	}
	return sequences.at(0);
}

const AnimationDirection& Animation::getDirection(const String& dirName) const
{
	Expects(directions.size() > 0);

	for (auto& dir : directions) {
		if (dir.name == dirName) {
			return dir;
		}
	}
	return directions[0];
}

const AnimationDirection& Animation::getDirection(int id) const
{
	Expects(id >= 0);
	Expects(directions.size() > 0);

	if (id < int(directions.size())) {
		return directions[id];
	} else {
		return directions[0];
	}
}

std::vector<String> Animation::getSequenceNames() const
{
	std::vector<String> result;
	for (auto& s: sequences) {
		result.emplace_back(s.getName());
	}
	return result;
}

std::vector<String> Animation::getDirectionNames() const
{
	std::vector<String> result;
	for (auto& d: directions) {
		result.emplace_back(d.getName());
	}
	return result;
}

Vector2i Animation::getPivot() const
{
	return sequences.at(0).getFrame(0).getSprite(0).origPivot;
}

Rect4i Animation::getBounds() const
{
	auto& sprite = sequences.at(0).getFrame(0).getSprite(0);
	const auto size = Vector2i(sprite.size) + Vector2i(sprite.trimBorder.xy() + sprite.trimBorder.zw());
	const auto pivot = sprite.origPivot;
	return Rect4i(-pivot, -pivot + size);
}

bool Animation::hasSequence(const String& seqName) const
{
	for (auto& s: sequences) {
		if (s.getName() == seqName) {
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
