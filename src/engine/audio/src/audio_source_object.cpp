#include "audio_source_object.h"

#include "audio_clip.h"
#include "audio_engine.h"
#include "audio_filter_resample.h"
#include "audio_object.h"
#include "audio_source_clip.h"

using namespace Halley;

AudioSourceObject::AudioSourceObject(AudioEngine& engine, std::shared_ptr<const AudioObject> obj)
	: object(std::move(obj))
{
	auto& rng = engine.getRNG();
	auto clip = object->getRandomClip(rng);

	if (clip) {
		const auto volume = object->getVolume();
		const auto pitch = object->getPitch();
		const float curVolume = rng.getFloat(volume.start, volume.end);
		const float curPitch = clamp(rng.getFloat(pitch.start, pitch.end), 0.1f, 2.0f);

		constexpr int sampleRate = 48000;
		source = std::make_shared<AudioSourceClip>(std::move(clip), object->getLoop(), lround(object->getDelay() * sampleRate));
		if (std::abs(curPitch - 1.0f) > 0.01f) {
			source = std::make_shared<AudioFilterResample>(source, int(lround(sampleRate * curPitch)), sampleRate, engine.getPool());
		}
	}
}

uint8_t AudioSourceObject::getNumberOfChannels() const
{
	if (!source) {
		return 0;
	}
	return source->getNumberOfChannels();
}

bool AudioSourceObject::getAudioData(size_t numSamples, AudioSourceData& dst)
{
	if (!source) {
		return false;
	}
	return source->getAudioData(numSamples, dst);
}

bool AudioSourceObject::isReady() const
{
	if (!source) {
		return true;
	}
	return source->isReady();
}

