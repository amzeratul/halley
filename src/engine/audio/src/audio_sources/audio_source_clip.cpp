#include "audio_source_clip.h"
#include <utility>
#include "audio_clip.h"

using namespace Halley;


AudioSourceClip::AudioSourceClip(std::shared_ptr<const IAudioClip> c, bool looping, int64_t delaySamples, int64_t loopStart, int64_t loopEnd)
	: clip(std::move(c))
	, playbackPos(-delaySamples)
	, loopStart(loopStart)
	, loopEnd(loopEnd)
	, looping(looping)
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

bool AudioSourceClip::getAudioData(size_t samplesRequested, AudioSourceData dstChannels)
{
	Expects(isReady());
	if (!initialised) {
		initialised = true;
	}
	const auto playbackLength = static_cast<int64_t>(clip->getLength());

	bool isPlaying = true;
	const uint8_t nChannels = getNumberOfChannels();
	size_t samplesWritten = 0;

	// On delay, pad with zeroes
	if (playbackPos < 0) {
		const size_t delaySamples = std::min(static_cast<size_t>(-playbackPos), samplesRequested);

		for (size_t srcChannel = 0; srcChannel < nChannels; ++srcChannel) {
			auto dstBuf = dstChannels[srcChannel];
			auto dst = dstBuf.data() + samplesWritten;
			memset(dst, 0, delaySamples * sizeof(AudioSample));
		}

		samplesWritten += delaySamples;
		playbackPos += static_cast<int64_t>(delaySamples);
	}

	// Playback pos is positive
	while (samplesWritten < samplesRequested) {
		if (playbackPos >= playbackLength) {
			// If we're at the end of playback, either loop, or flag as done
			if (looping) {
				playbackPos = std::max(loopStart, static_cast<int64_t>(clip->getLoopPoint()));
				if (playbackPos >= playbackLength) {
					looping = false;
					playbackPos = playbackLength;
					isPlaying = false;
				}
			} else {
				isPlaying = false;
			}
		}

		const size_t samplesAvailable = static_cast<size_t>(playbackLength - playbackPos);
		const size_t samplesRemaining = samplesRequested - samplesWritten;
		const size_t samplesToRead = std::min(samplesRemaining, samplesAvailable);

		if (samplesToRead > 0) {
			// We have some samples that we can read, so go ahead with reading them
			for (size_t srcChannel = 0; srcChannel < nChannels; ++srcChannel) {
				auto dst = AudioSamples(dstChannels[srcChannel].data() + samplesWritten, samplesToRead);
				const size_t nCopied = clip->copyChannelData(srcChannel, static_cast<size_t>(playbackPos), samplesToRead, dst);
				Expects(nCopied <= samplesRequested * sizeof(AudioSample));
			}

			playbackPos += static_cast<int64_t>(samplesToRead);
			samplesWritten += samplesToRead;
		} else {
			// Reached end of playback, pad with zeroes
			for (size_t srcChannel = 0; srcChannel < nChannels; ++srcChannel) {
				auto dstBuf = dstChannels[srcChannel];
				auto dst = dstBuf.data() + samplesWritten;
				memset(dst, 0, samplesRemaining * sizeof(AudioSample));
			}

			samplesWritten += samplesRemaining;
		}
	}

	return isPlaying;
}
