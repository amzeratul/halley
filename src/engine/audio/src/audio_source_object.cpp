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

	if (auto clip = object->getRandomClip(rng); clip) {
		source = std::make_shared<AudioSourceClip>(std::move(clip), object->getLoop(), lround(object->getDelay() * AudioConfig::sampleRate));
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

