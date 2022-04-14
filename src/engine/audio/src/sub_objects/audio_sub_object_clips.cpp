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
	delay = node["delay"].asFloat(0.0f);
	loop = node["loop"].asBool(false);
	loopStart = node["loopStart"].asInt();
	loopEnd = node["loopEnd"].asInt();
}

std::unique_ptr<AudioSource> AudioSubObjectClips::makeSource(AudioEngine& engine, AudioEmitter& emitter) const
{
	if (clipData.empty()) {
		return {};
	}

	auto clip = engine.getRNG().getRandomElement(clipData);
	return std::make_unique<AudioSourceClip>(clip, loop, lroundl(delay * AudioConfig::sampleRate), loopStart, loopEnd);
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
	s << delay;
	s << loop;
}

void AudioSubObjectClips::deserialize(Deserializer& s)
{
	s >> clips;
	s >> delay;
	s >> loop;
}
