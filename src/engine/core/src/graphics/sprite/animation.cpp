#include "halley/graphics/sprite/animation.h"
#include "halley/graphics/sprite/sprite_sheet.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/material/material_parameter.h"
#include "halley/api/halley_api.h"
#include "halley/resources/resources.h"
#include "halley/bytes/byte_serializer.h"
#include <gsl/gsl_assert>
#include <utility>

#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

AnimationFrame::AnimationFrame(int frameNumber, int duration, const String& imageName, const SpriteSheet& sheet, const Vector<AnimationDirection>& directions)
	: duration(duration)
{
	const size_t n = directions.size();
	sprites.resize(n);
	for (size_t i = 0; i < n; i++) {
		const auto& name = directions[i].needsToProcessFrameName(imageName) ? directions[i].getFrameName(frameNumber, imageName) : imageName;
		sprites[i] = sheet.tryGetSprite(name);
		if (!sprites[i]) {
			sprites[i] = &sheet.getDummySprite();
			Logger::logWarning("Missing animation frame: " + name);
		}
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

AnimationSequence::AnimationSequence(String name, bool loop, bool noFlip, bool fallback)
	: name(std::move(name))
	, id(-1)
	, loop(loop)
	, noFlip(noFlip)
	, fallback(fallback)
{}

void AnimationSequence::serialize(Serializer& s) const
{
	s << frameDefinitions;
	s << name;
	s << id;
	s << loop;
	s << noFlip;
	s << fallback;
}

void AnimationSequence::deserialize(Deserializer& s)
{
	s >> frameDefinitions;
	s >> name;
	s >> id;
	s >> loop;
	s >> noFlip;
	s >> fallback;
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

AnimationDirection::AnimationDirection(String name, String fileName, bool flip)
	: name(std::move(name))
	, fileName(std::move(fileName))
	, id(-1)
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


AnimationActionPoint::AnimationActionPoint(const ConfigNode& config, String name, int id, gsl::span<const AnimationSequence> sequences, gsl::span<const AnimationDirection> directions)
	: name(std::move(name))
	, id(id)
{
	for (const auto& [k, v]: config.asMap()) {
		const auto split = k.split(':');
		const auto& seqName = split[0];
		const auto& dirName = split[1];

		const auto seqIter = std_ex::find_if(sequences, [&](const AnimationSequence& seq) { return seq.getName() == seqName; });
		const auto dirIter = std_ex::find_if(directions, [&](const AnimationDirection& dir) { return dir.getName() == dirName; });

		if (seqIter != sequences.end() && dirIter != directions.end()) {
			const int seqIdx = static_cast<int>(seqIter - sequences.begin());
			const int dirIdx = static_cast<int>(dirIter - directions.begin());

			for (int i = 0; i < static_cast<int>(v.asSequence().size()); ++i) {
				auto pos = v[i].asVector2i();
				if (directions[dirIdx].shouldFlip()) {
					pos.x *= -1;
				}
				points[std::tuple<int, int, int>(seqIdx, dirIdx, i)] = pos;
			}
		}
	}
}

std::optional<Vector2i> AnimationActionPoint::getPoint(int sequenceIdx, int directionIdx, int frameNumber) const
{
	const auto iter = points.find(std::tuple<int, int, int>(sequenceIdx, directionIdx, frameNumber));
	if (iter != points.end()) {
		return iter->second;
	}

	return {};
}

void AnimationActionPoint::serialize(Serializer& s) const
{
	s << name;
	s << id;
	s << points;
}

void AnimationActionPoint::deserialize(Deserializer& s)
{
	s >> name;
	s >> id;
	s >> points;
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
	material = spriteSheet->getMaterial(materialName);

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

void Animation::addSequence(AnimationSequence sequence)
{
	sequence.id = static_cast<int>(sequences.size());
	sequences.push_back(std::move(sequence));
}

void Animation::addDirection(AnimationDirection direction, std::optional<int> idx)
{
	direction.id = idx.value_or(static_cast<int>(directions.size()));
	directions.push_back(std::move(direction));
}

void Animation::addActionPoints(const ConfigNode& config)
{
	int idx = static_cast<int>(actionPoints.size());
	if (config.getType() == ConfigNodeType::Map) {
		for (const auto& [k, v]: config.asMap()) {
			actionPoints.emplace_back(v, k, idx++, sequences, directions);
		}
	}
}

const AnimationSequence& Animation::getSequence(const String& seqName) const
{
	const AnimationSequence* fallback = nullptr;

	for (auto& seq: sequences) {
		if (seq.name == seqName) {
			return seq;
		}
		if (seq.isFallback()) {
			fallback = &seq;
		}
	}

	if (fallback) {
		return *fallback;
	} else {
		return sequences.at(0);
	}
}

const AnimationSequence& Animation::getSequence(size_t idx) const
{
	return sequences.at(idx);
}

size_t Animation::getSequenceIdx(const String& name) const
{
	size_t fallback = 0;
	size_t i = 0;
	for (const auto& seq: sequences) {
		if (seq.name == name) {
			return i;
		}
		if (seq.isFallback()) {
			fallback = i;
		}
	}

	return fallback;
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

Vector<String> Animation::getSequenceNames() const
{
	Vector<String> result;
	for (auto& s: sequences) {
		result.emplace_back(s.getName());
	}
	return result;
}

Vector<String> Animation::getDirectionNames() const
{
	Vector<String> result;
	for (auto& d: directions) {
		result.emplace_back(d.getName());
	}
	return result;
}

std::optional<Vector2i> Animation::getActionPoint(const String& actionPoint, const String& sequenceName, const String& directionName, int frameNumber) const
{
	int sequenceIndex = -1;
	for (int i = 0; i < static_cast<int>(sequences.size()); ++i) {
		if (sequences[i].getName() == sequenceName) {
			sequenceIndex = i;
			break;
		}
	}
	if (sequenceIndex == -1) {
		return {};
	}

	int directionIndex = -1;
	for (int i = 0; i < static_cast<int>(directions.size()); ++i) {
		if (directions[i].getName() == directionName) {
			directionIndex = i;
			break;
		}
	}
	if (directionIndex == -1) {
		return {};
	}

	return getActionPoint(actionPoint, sequenceIndex, directionIndex, frameNumber);
}

std::optional<Vector2i> Animation::getActionPoint(const String& actionPoint, int sequenceIdx, int directionIdx, int frameNumber) const
{
	for (const auto& ap: actionPoints) {
		if (ap.getName() == actionPoint) {
			auto p = ap.getPoint(sequenceIdx, directionIdx, frameNumber);
			if (p) {
				return *p - getPivot();
			}
		}
	}
	return {};
}

Vector2i Animation::getPivot() const
{
	if (!hasPivot) {
		pivot = sequences.at(0).getFrame(0).getSprite(0).origPivot;
		hasPivot = true;
	}
	return pivot;
}

Rect4i Animation::getBounds() const
{
	if (!hasBounds) {
		auto& sprite = sequences.at(0).getFrame(0).getSprite(0);
		const auto size = Vector2i(sprite.size) + Vector2i(sprite.trimBorder.xy() + sprite.trimBorder.zw());
		const auto pivot = sprite.origPivot;
		bounds = Rect4i(-pivot, -pivot + size);
		hasBounds = true;
	}
	return bounds;
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
	s << actionPoints;
}

void Animation::deserialize(Deserializer& s)
{
	s >> name;
	s >> spriteSheetName;
	s >> materialName;
	s >> sequences;
	s >> directions;
	s >> actionPoints;
}
