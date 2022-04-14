#include "audio_source_layers.h"

#include "audio_clip.h"
#include "../audio_engine.h"
#include "../audio_filter_resample.h"
#include "audio_object.h"
#include "audio_source_clip.h"

using namespace Halley;

AudioSourceLayers::AudioSourceLayers(AudioEngine& engine, std::shared_ptr<const AudioObject> obj)
	: object(std::move(obj))
{
}

uint8_t AudioSourceLayers::getNumberOfChannels() const
{
	return 0;
}

bool AudioSourceLayers::getAudioData(size_t numSamples, AudioSourceData& dst)
{
	return false;
}

bool AudioSourceLayers::isReady() const
{
	return true;
}

