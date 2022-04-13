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
}

uint8_t AudioSourceObject::getNumberOfChannels() const
{
	return 0;
}

bool AudioSourceObject::getAudioData(size_t numSamples, AudioSourceData& dst)
{
	return false;
}

bool AudioSourceObject::isReady() const
{
	return true;
}

