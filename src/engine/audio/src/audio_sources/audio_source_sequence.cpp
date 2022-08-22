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

	const auto nChannels = getNumberOfChannels();
	size_t samplePos = 0;
	const auto fadeLen = static_cast<size_t>(sequenceConfig.getCrossFade().getLength() * AudioConfig::sampleRate);

	while (samplePos < samplesRequested) {
		if (playingTracks.empty()) {
			AudioMixer::zeroRange(dst, nChannels, samplePos);
			return false;
		}

		// Figure out how many samples we'll read
		size_t samplesToRead = samplesRequested - samplePos;
		for (const auto& p: playingTracks) {
			samplesToRead = std::min(samplesToRead, p.getSamplesBeforeNextEvent(fadeLen));
		}

		// Transition out anyone who needs it
		size_t startNewTrack = 0;
		size_t nPlaying = 0;
		for (auto& p: playingTracks) {
			if (p.state != TrackState::Done) {
				if (p.getSamplesBeforeEnd() == samplesToRead) {
					if (p.state == TrackState::Playing) {
						++startNewTrack;
					}
					p.state = TrackState::Done;
				} else if (p.getSamplesBeforeTransition(fadeLen) == samplesToRead) {
					if (p.state == TrackState::Playing) {
						++startNewTrack;
					}
					p.state = TrackState::TransitioningOut;
				}
				if (p.getSamplesBeforeEnd() == fadeLen) {
					p.prevGain = p.fader.getCurrentValue();
					p.fader.startFade(p.fader.getCurrentValue(), 0, sequenceConfig.getCrossFade());
				}
				++nPlaying;
			}
		}

		// Read samples
		if (nPlaying == 1 && samplePos == 0 && samplesToRead == samplesRequested && playingTracks.front().fader.getCurrentValue() == 1 && playingTracks.front().prevGain == 1) {
			// Passthrough!
			playingTracks.front().source->getAudioData(samplesRequested, dst);
		} else {
			// Mix tracks
			auto buffer = engine.getPool().getBuffers(nChannels, samplesRequested);
			bool first = true;
			for (auto& p: playingTracks) {
				p.source->getAudioData(samplesToRead, buffer.getSampleSpans());
				const float gain = lerp(p.prevGain, p.fader.getCurrentValue(), static_cast<float>(samplePos) / static_cast<float>(samplesRequested));

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

		// Update fades
		const float time = static_cast<float>(samplesToRead) / static_cast<float>(AudioConfig::sampleRate);
		for (auto& p: playingTracks) {
			p.prevGain = p.fader.getCurrentValue();
			p.fader.update(time);
		}
		
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

void AudioSourceSequence::restart()
{
	playingTracks.clear();
	initialize();
}

size_t AudioSourceSequence::PlayingTrack::getSamplesBeforeNextEvent(size_t fadeLen) const
{
	const auto left = source->getSamplesLeft();
	const auto e0 = endSamplesOverlap + fadeLen;
	const auto e1 = endSamplesOverlap;
	return left > e0 ? left - e0 : (left > e1 ? left - e1 : left);
}

size_t AudioSourceSequence::PlayingTrack::getSamplesBeforeTransition(size_t fadeLen) const
{
	const auto left = source->getSamplesLeft();
	size_t overlap = endSamplesOverlap + fadeLen;
	return left > overlap ? left - overlap : 0;
}

size_t AudioSourceSequence::PlayingTrack::getSamplesBeforeEnd() const
{
	return source->getSamplesLeft();
}

void AudioSourceSequence::initialize()
{
	initialized = true;

	playList.clear();
	for (size_t i = 0; i < sequenceConfig.getNumSubObjects(); ++i) {
		playList.push_back(i);
	}

	const auto type = sequenceConfig.getSequenceType();
	if (type == AudioSequenceType::Shuffle || type == AudioSequenceType::ShuffleOnce) {
		shuffle(playList.begin(), playList.end(), engine.getRNG());
	}

	if (type == AudioSequenceType::Random) {
		curTrack = engine.getRNG().getSizeT(0, playList.size() - 1);
	} else {
		curTrack = 0;
	}

	loadCurrentTrack();
}

void AudioSourceSequence::nextTrack()
{
	if (sequenceConfig.getSequenceType() == AudioSequenceType::Random) {
		curTrack = engine.getRNG().getSizeT(0, playList.size() - 1);
	} else {
		++curTrack;

		if (curTrack == playList.size()) {
			curTrack = 0;
			if (sequenceConfig.getSequenceType() == AudioSequenceType::Shuffle) {
				const auto lastTrack = playList.back();
				shuffle(playList.begin(), playList.end(), engine.getRNG());
				if (playList.front() == lastTrack && playList.size() >= 2) {
					// If we got the last track as the start, swap it with another random track
					const auto idx = engine.getRNG().getSizeT(1, playList.size() - 1);
					std::swap(playList.front(), playList[idx]);
				}
			}
		}
	}

	loadCurrentTrack();
}

void AudioSourceSequence::loadCurrentTrack()
{
	const auto& segment = sequenceConfig.getSegments()[playList[curTrack]];
	playingTracks.push_back({ segment.object->makeSource(engine, emitter) });
	auto& track = playingTracks.back();
	track.fader.startFade(0.0f, 1.0f, sequenceConfig.getCrossFade());
	track.prevGain = track.fader.getCurrentValue();

	const auto nSamples = track.source->getSamplesLeft();
	if (segment.endSample > 0 && static_cast<size_t>(segment.endSample) < nSamples) {
		track.endSamplesOverlap = nSamples - static_cast<size_t>(segment.endSample);
	}
}
