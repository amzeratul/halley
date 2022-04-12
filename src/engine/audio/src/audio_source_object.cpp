#include "audio_source_object.h"

using namespace Halley;

AudioSourceObject::AudioSourceObject(std::shared_ptr<const AudioObject> object)
	: object(std::move(object))
{
}

uint8_t AudioSourceObject::getNumberOfChannels() const
{
	// TODO
	return 0;
}

bool AudioSourceObject::getAudioData(size_t numSamples, AudioSourceData& dst)
{
	// TODO
	return false;
}

bool AudioSourceObject::isReady() const
{
	// TODO
	return false;
}
