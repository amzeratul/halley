#include "audio_sub_object_clips.h"

#include "halley/core/resources/resources.h"
#include "halley/support/logger.h"
#include "halley/bytes/byte_serializer.h"
#include "../audio_engine.h"
#include "../audio_sources/audio_source_clip.h"
#include "audio_clip.h"

namespace Halley {
	class AudioSourceClip;
}

using namespace Halley;


void AudioSubObjectClips::load(const ConfigNode& node)
{
	clips = node["clips"].asVector<String>({});
	loop = node["loop"].asBool(false);
	loopStart = node["loopStart"].asInt(0);
	loopEnd = node["loopEnd"].asInt(0);
}

ConfigNode AudioSubObjectClips::toConfigNode() const
{
	ConfigNode::MapType result;

	if (!clips.empty()) {
		result["clips"] = clips;
	}
	if (loop) {
		result["loop"] = loop;
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
	return std::make_unique<AudioSourceClip>(clip, loop, loopStart, loopEnd);
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
}

void AudioSubObjectClips::serialize(Serializer& s) const
{
	s << clips;
	s << loop;
}

void AudioSubObjectClips::deserialize(Deserializer& s)
{
	s >> clips;
	s >> loop;
}

void AudioSubObjectClips::addClip(std::shared_ptr<const AudioClip> audioClip, size_t idx)
{
	const auto pos = std::min(clips.size(), idx);
	clips.insert(clips.begin() + pos, audioClip->getAssetId());
	clipData.insert(clipData.begin() + pos, std::move(audioClip));
}
