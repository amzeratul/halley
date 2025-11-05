#include "audio_source_object.h"

#include "halley/audio/audio_object.h"
using namespace Halley;

AudioSourceObject::AudioSourceObject(AudioEngine& engine, AudioEmitter& emitter, const AudioObject& object)
	: object(object)
{
	sources.reserve(object.getSubObjects().size());
	for (const auto& subObject: object.getSubObjects()) {
		if (auto source = subObject->makeSource(engine, emitter)) {
			sources.emplace_back(std::move(source));
		}
	}

	if (sources.size() >= 2) {
		for (size_t i = 1; i < sources.size(); ++i) {
			if (sources[i]->getNumberOfChannels() != sources[0]->getNumberOfChannels()) {
				Logger::logError("AudioSourceObject has inconsistent channel counts: " + object.getAssetId());
			}
		}
	}
}

String AudioSourceObject::getName() const
{
	return object.getAssetId();
}

bool AudioSourceObject::isReady() const
{
	if (sources.empty()) {
		return false;
	}

	for (const auto& src: sources) {
		if (!src->isReady()) {
			return false;
		}
	}
	return true;
}

uint8_t AudioSourceObject::getNumberOfChannels() const
{
	return sources.empty() ? 0 : sources[0]->getNumberOfChannels();
}

size_t AudioSourceObject::getSamplesLeft() const
{
	size_t left = 0;
	for (const auto& src: sources) {
		left = std::max(left, src->getSamplesLeft());
	}
	return left;
}

bool AudioSourceObject::getAudioData(size_t numSamples, AudioMultiChannelSamples dst)
{
	if (sources.size() == 1) {
		return sources[0]->getAudioData(numSamples, dst);
	} else if (!sources.empty()) {
		// TODO: read and mix all sources
		return sources[0]->getAudioData(numSamples, dst);
	} else {
		return false;
	}
}

void AudioSourceObject::restart()
{
	for (auto& src: sources) {
		src->restart();
	}
}

bool AudioSourceObject::isLooping()
{
	for (const auto& src: sources) {
		if (src->isLooping()) {
			return true;
		}
	}
	return false;
}
