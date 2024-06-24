#include "halley/audio/sub_objects/audio_sub_object_clips.h"

#include "halley/resources/resources.h"
#include "halley/support/logger.h"
#include "halley/bytes/byte_serializer.h"
#include "../audio_engine.h"
#include "../audio_sources/audio_source_clip.h"
#include "halley/audio/audio_clip.h"

namespace Halley {
	class AudioSourceClip;
}

using namespace Halley;


AudioSubObjectClips::AudioSubObjectClips(std::shared_ptr<const AudioClip> clip)
{
	clipData.push_back(std::move(clip));
	clips.push_back(clipData.back()->getAssetId());
}

void AudioSubObjectClips::load(const ConfigNode& node)
{
	clips = node["clips"].asVector<String>({});
	loop = node["loop"].asBool(false);
	randomiseStart = node["randomiseStart"].asBool(false);
	gain = node["gain"].asFloatRange(Range<float>(1, 1));
	loopStart = node["loopStart"].asInt(0);
	loopEnd = node["loopEnd"].asInt(0);
}

ConfigNode AudioSubObjectClips::toConfigNode() const
{
	ConfigNode::MapType result;

	result["type"] = toString(getType());
	if (!clips.empty()) {
		result["clips"] = clips;
	}
	if (loop) {
		result["loop"] = loop;
	}
	if (randomiseStart) {
		result["randomiseStart"] = randomiseStart;
	}
	if (gain != Range<float>(1, 1)) {
		result["gain"] = gain;
	}
	if (loopStart != 0) {
		result["loopStart"] = loopStart;
	}
	if (loopEnd != 0) {
		result["loopEnd"] = loopEnd;
	}
		
	return result;
}

void AudioSubObjectClips::toLegacyConfigNode(ConfigNode& dst) const
{
	dst["clips"] = clips;
	if (loop) {
		dst["loop"] = loop;
	}
}

String AudioSubObjectClips::getName() const
{
	if (clips.size() == 1) {
		return clips[0];
	}
	return "Clips";
}

gsl::span<const String> AudioSubObjectClips::getClips() const
{
	return clips;
}

bool AudioSubObjectClips::canCollapseToClip() const
{
	return true;
}

std::unique_ptr<AudioSource> AudioSubObjectClips::makeSource(AudioEngine& engine, AudioEmitter& emitter) const
{
	if (clipData.empty()) {
		return {};
	}

	auto clip = engine.getRNG().getRandomElement(clipData);
	return std::make_unique<AudioSourceClip>(engine, clip, loop, engine.getRNG().getFloat(gain), loopStart, loopEnd, randomiseStart);
}

void AudioSubObjectClips::loadDependencies(Resources& resources)
{
	if (clipData.size() != clips.size()) {
		clipData.clear();
		clipData.reserve(clips.size());

		for (auto& c: clips) {
			if (resources.exists<AudioClip>(c)) {
				clipData.push_back(resources.get<AudioClip>(c));
			} else {
				Logger::logError("AudioClip not found: \"" + c + "\".");
				clipData.push_back(std::shared_ptr<AudioClip>());
			}
		}
	}
	depsLoaded = true;
}

void AudioSubObjectClips::serialize(Serializer& s) const
{
	s << clips;
	s << loop;
	s << randomiseStart;
	s << gain;
	s << loopStart;
	s << loopEnd;
}

void AudioSubObjectClips::deserialize(Deserializer& s)
{
	s >> clips;
	s >> loop;
	s >> randomiseStart;
	s >> gain;
	s >> loopStart;
	s >> loopEnd;
}

bool AudioSubObjectClips::canAddObject(AudioSubObjectType type, const std::optional<String>& caseName) const
{
	return type == AudioSubObjectType::Clips;
}

void AudioSubObjectClips::addObject(AudioSubObjectHandle object, const std::optional<String>& caseName, size_t idx)
{
	auto& other = dynamic_cast<AudioSubObjectClips&>(object.getObject());
	const auto pos = std::min(clips.size(), idx);
	clips.insert(clips.begin() + pos, other.clips.begin(), other.clips.end());
	if (depsLoaded) {
		clipData.insert(clipData.begin() + pos, other.clipData.begin(), other.clipData.end());
	}
}

void AudioSubObjectClips::addClip(std::shared_ptr<const AudioClip> audioClip, const std::optional<String>& caseName, size_t idx)
{
	const auto pos = std::min(clips.size(), idx);
	clips.insert(clips.begin() + pos, audioClip->getAssetId());
	if (depsLoaded) {
		clipData.insert(clipData.begin() + pos, std::move(audioClip));
	}
}

void AudioSubObjectClips::removeClip(const String& clipId)
{
	const auto iter = std::find(clips.begin(), clips.end(), clipId);
	if (iter != clips.end()) {
		const auto pos = iter - clips.begin();
		clips.erase(iter);
		if (depsLoaded) {
			clipData.erase(clipData.begin() + pos);
		}
	}
}

void AudioSubObjectClips::swapClips(size_t idxA, size_t idxB)
{
	std::swap(clips[idxA], clips[idxB]);
	if (depsLoaded) {
		std::swap(clipData[idxA], clipData[idxB]);
	}
}

bool AudioSubObjectClips::getLoop() const
{
	return loop;
}

void AudioSubObjectClips::setLoop(bool value)
{
	loop = value;
}

Range<float>& AudioSubObjectClips::getGain()
{
	return gain;
}

int AudioSubObjectClips::getLoopStart() const
{
	return loopStart;
}

int AudioSubObjectClips::getLoopEnd() const
{
	return loopEnd;
}

void AudioSubObjectClips::setLoopStart(int samples)
{
	loopStart = samples;
}

void AudioSubObjectClips::setLoopEnd(int samples)
{
	loopEnd = samples;
}

bool AudioSubObjectClips::getRandomiseStart() const
{
	return randomiseStart;
}

void AudioSubObjectClips::setRandomiseStart(bool enabled)
{
	randomiseStart = enabled;
}
