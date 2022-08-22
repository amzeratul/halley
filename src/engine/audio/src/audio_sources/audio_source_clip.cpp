#include "audio_source_clip.h"
#include <utility>
#include "audio_clip.h"
#include "../audio_mixer.h"
#include "../audio_engine.h"

using namespace Halley;


AudioSourceClip::AudioSourceClip(AudioEngine& engine, std::shared_ptr<const IAudioClip> c, bool looping, float gain, int64_t loopStart, int64_t loopEnd, bool randomiseStart)
	: engine(engine)
	, clip(std::move(c))
	, loopStart(loopStart)
	, loopEnd(loopEnd)
	, gain(gain)
	, prevGain(gain)
	, looping(looping)
	, randomiseStart(randomiseStart)
{
	Expects(clip != nullptr);
}

uint8_t AudioSourceClip::getNumberOfChannels() const
{
	return clip->getNumberOfChannels();
}

bool AudioSourceClip::isReady() const
{
	return clip->isLoaded();
}

size_t AudioSourceClip::getSamplesLeft() const
{
	return looping ? std::numeric_limits<size_t>::max() : (clip->getLength() - streams[0].playbackPos);
}

void AudioSourceClip::restart()
{
	streams[0] = {};
	streams[1] = {};
	initialised = false;
}

bool AudioSourceClip::getAudioData(size_t samplesRequested, AudioMultiChannelSamples dstChannels)
{
	Expects(isReady());

	// Set stream end positions
	const auto clipLength = clip->getLength();
	const bool hasEarlyEnd = loopEnd > 0 && static_cast<size_t>(loopEnd) < clipLength;
	streams[0].endPos = hasEarlyEnd ? static_cast<size_t>(loopEnd) : clipLength;
	streams[0].kickOffSecondStream = hasEarlyEnd;
	streams[1].endPos = clipLength;

	if (!initialised) {
		initialised = true;

		streams[0].active = true;
		streams[0].loop = looping;
		streams[0].playbackPos = looping && randomiseStart ? engine.getRNG().getSizeT(0, streams[0].endPos) : 0;
	}

	const uint8_t nChannels = getNumberOfChannels();
	size_t samplesWritten = 0;

	while (samplesWritten < samplesRequested) {
		for (auto& stream: streams) {
			if (stream.active) {
				if (stream.playbackPos >= stream.endPos) {
					// If we're at the end of playback, either loop, or flag as done
					if (stream.loop) {
						const auto prevPos = stream.playbackPos;
						stream.playbackPos = std::max(static_cast<size_t>(loopStart), clip->getLoopPoint());
						if (stream.playbackPos >= clipLength) {
							// Loop failed
							looping = false;
							stream.playbackPos = clipLength;
							stream.active = false;
						} else {
							// Loop ok
							if (stream.kickOffSecondStream) {
								streams[1].active = true;
								streams[1].playbackPos = prevPos;
							}
						}
					} else {
						stream.active = false;
					}
				}
			}
		}

		size_t streamsActive = 0;
		size_t samplesAvailable = std::numeric_limits<size_t>::max();
		for (const auto& stream: streams) {
			if (stream.active) {
				samplesAvailable = std::min(samplesAvailable, stream.endPos - stream.playbackPos);
				++streamsActive;
			}
		}
		if (streamsActive == 0) {
			samplesAvailable = 0;
		}

		const size_t samplesRemaining = samplesRequested - samplesWritten;
		const size_t samplesToRead = std::min(samplesRemaining, samplesAvailable);

		if (samplesToRead > 0) {
			// We have some samples that we can read, so go ahead with reading them
			bool first = true;

			for (auto& stream: streams) {
				if (stream.active) {
					if (first) {
						for (size_t ch = 0; ch < nChannels; ++ch) {
							auto dst = dstChannels[ch].subspan(samplesWritten, samplesToRead);
							const size_t nCopied = clip->copyChannelData(ch, stream.playbackPos, samplesToRead, prevGain, gain, dst);
							assert(nCopied <= samplesRequested * sizeof(AudioSample));
						}
						first = false;
					} else {
						auto buffer = engine.getPool().getBuffer(samplesToRead);
						for (size_t ch = 0; ch < nChannels; ++ch) {
							auto dst = dstChannels[ch].subspan(samplesWritten, samplesToRead);
							const size_t nCopied = clip->copyChannelData(ch, stream.playbackPos, samplesToRead, prevGain, gain, buffer.getSpan());
							AudioMixer::mixAudio(buffer.getSpan(), dst, 1, 1);
							assert(nCopied <= samplesRequested * sizeof(AudioSample));
						}
					}

					stream.playbackPos += static_cast<int64_t>(samplesToRead);
				}
			}

			samplesWritten += samplesToRead;
		} else {
			// Reached end of playback, pad with zeroes
			AudioMixer::zeroRange(dstChannels, nChannels, samplesWritten, samplesRemaining);
			samplesWritten += samplesRemaining;
		}
	}

	prevGain = gain;

	return std::any_of(streams.begin(), streams.end(), [] (const auto& s) { return s.active; });
}
