#include "audio_source_sequence.h"

#include "../audio_engine.h"
#include "../audio_mixer.h"
#include "halley/utils/algorithm.h"
using namespace Halley;

AudioSourceSequence::AudioSourceSequence(AudioEngine& engine, AudioEmitter& emitter, const AudioSubObjectSequence& sequenceConfig)
	: engine(engine)
	, emitter(emitter)
	, sequenceConfig(sequenceConfig)
{
	initialize();
}

uint8_t AudioSourceSequence::getNumberOfChannels() const
{
	return playingTracks.empty() ? 0 : playingTracks.front().source->getNumberOfChannels();
}

bool AudioSourceSequence::getAudioData(size_t samplesRequested, AudioMultiChannelSamples dst)
{
	if (playingTracks.empty()) {
		AudioMixer::zero(dst);
		return false;
	}

	// Update gains
	for (auto& p: playingTracks) {
		p.prevGain = p.gain;
	}

	const auto nChannels = getNumberOfChannels();
	size_t samplePos = 0;

	while (samplePos < samplesRequested) {
		if (playingTracks.empty()) {
			AudioMixer::zeroRange(dst, nChannels, samplePos);
			return false;
		}

		// Figure out how many samples we'll read
		size_t samplesToRead = samplesRequested - samplePos;
		for (const auto& p: playingTracks) {
			samplesToRead = std::min(samplesToRead, p.getSamplesBeforeTransition());
		}

		// Transition out anyone who needs it
		size_t startNewTrack = 0;
		size_t nPlaying = 0;
		for (auto& p: playingTracks) {
			if (p.state != TrackState::Done) {
				if (p.getSamplesBeforeEnd() == samplesToRead) {
					p.state = TrackState::TransitioningOut;
					++startNewTrack;
				} else if (p.getSamplesBeforeTransition() == samplesToRead) {
					p.state = TrackState::Done;
				}
				++nPlaying;
			}
		}

		// Read samples
		if (nPlaying == 1 && samplePos == 0 && samplesToRead == samplesRequested && playingTracks.front().gain == 1 && playingTracks.front().prevGain == 1) {
			// Passthrough!
			playingTracks.front().source->getAudioData(samplesRequested, dst);
		} else {
			// Mix tracks
			auto buffer = engine.getPool().getBuffers(nChannels, samplesRequested);
			bool first = true;
			for (auto& p: playingTracks) {
				p.source->getAudioData(samplesToRead, buffer.getSampleSpans());
				const float gain = lerp(p.prevGain, p.gain, static_cast<float>(samplePos) / static_cast<float>(samplesRequested));

				if (first) {
					for (size_t i = 0; i < nChannels; ++i) {
						AudioMixer::copy(dst[i].subspan(samplePos), buffer.getSampleSpans()[i], p.prevGain, gain);
					}
					first = false;
				} else {
					for (size_t i = 0; i < nChannels; ++i) {
						AudioMixer::mixAudio(buffer.getSampleSpans()[i], dst[i].subspan(samplePos), p.prevGain, gain);
					}
				}
			}
		}
		samplePos += samplesToRead;

		// Remove finished tracks
		std_ex::erase_if(playingTracks, [&] (const auto& p) { return p.state == TrackState::Done; });

		// Start new track(s)
		for (size_t i = 0; i < startNewTrack; ++i) {
			nextTrack();
		}
	}

	return true;
}

bool AudioSourceSequence::isReady() const
{
	for (auto& p: playingTracks) {
		if (!p.source->isReady()) {
			return false;
		}
	}
	return true;
}

size_t AudioSourceSequence::getSamplesLeft() const
{
	return std::numeric_limits<size_t>::max();
}

size_t AudioSourceSequence::PlayingTrack::getSamplesBeforeTransition() const
{
	const auto left = source->getSamplesLeft();
	return left > endSamplesOverlap ? left - endSamplesOverlap : 0;
}

size_t AudioSourceSequence::PlayingTrack::getSamplesBeforeEnd() const
{
	return source->getSamplesLeft();
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

	playingTracks.push_back({ sequenceConfig.getSubObject(playList[curTrack])->makeSource(engine, emitter) });
}
