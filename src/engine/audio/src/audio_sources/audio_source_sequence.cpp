#include "audio_source_sequence.h"

#include "../audio_engine.h"
#include "halley/utils/algorithm.h"
using namespace Halley;

AudioSourceSequence::AudioSourceSequence(AudioEngine& engine, AudioEmitter& emitter, const AudioSubObjectSequence& sequenceConfig)
	: engine(engine)
	, emitter(emitter)
	, sequenceConfig(sequenceConfig)
{
}

uint8_t AudioSourceSequence::getNumberOfChannels() const
{
	// TODO
	return 0;
}

bool AudioSourceSequence::getAudioData(size_t numSamples, AudioMultiChannelSamples dst)
{
	if (!initialized) {
		initialize();
	}

	// TODO
	return false;
}

bool AudioSourceSequence::isReady() const
{
	// TODO
	return false;
}

size_t AudioSourceSequence::getSamplesLeft() const
{
	return std::numeric_limits<size_t>::max();
}

void AudioSourceSequence::initialize()
{
	initialized = true;

	for (size_t i = 0; i < sequenceConfig.getNumSubObjects(); ++i) {
		playList.push_back(i);
	}

	const auto type = sequenceConfig.getSequenceType();
	if (type == AudioSequenceType::Shuffle || type == AudioSequenceType::ShuffleOnce) {
		shuffle(playList.begin(), playList.end(), engine.getRNG());
	}

	if (type == AudioSequenceType::Random) {
		curTrack = engine.getRNG().getSizeT(0, playList.size());
	} else {
		curTrack = 0;
	}
}

void AudioSourceSequence::nextTrack()
{
	if (sequenceConfig.getSequenceType() == AudioSequenceType::Random) {
		curTrack = engine.getRNG().getSizeT(0, playList.size());
	} else {
		++curTrack;

		if (curTrack == playList.size()) {
			curTrack = 0;
			if (sequenceConfig.getSequenceType() == AudioSequenceType::Shuffle) {
				const auto lastTrack = playList.back();
				shuffle(playList.begin(), playList.end(), engine.getRNG());
				if (playList.front() == lastTrack && playList.size() >= 2) {
					// If we got the last track as the start, swap it with another random track
					std::swap(playList.front(), playList[engine.getRNG().getSizeT(1, playList.size())]);
				}
			}
		}
	}
}
