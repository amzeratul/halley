#include "audio_source_clip.h"
#include <utility>
#include "audio_clip.h"

using namespace Halley;


AudioSourceClip::AudioSourceClip(std::shared_ptr<const IAudioClip> c, bool looping)
	: clip(std::move(c))
	, looping(looping)
{
	Expects(clip);
	playbackPos = 0;
	playbackLength = 0;
}

size_t AudioSourceClip::getNumberOfChannels() const
{
	return clip->getNumberOfChannels();
}

bool AudioSourceClip::isReady() const
{
	return clip->isLoaded();
}

bool AudioSourceClip::getAudioData(size_t samplesRequested, AudioSourceData& dstChannels)
{
	if (!initialised) {
		initialised = true;
	}
	playbackLength = clip->getLength();
	Expects(playbackPos <= playbackLength);

	bool isPlaying = true;
	const size_t nChannels = getNumberOfChannels();
	size_t samplesWritten = 0;

	while (samplesWritten < samplesRequested) {
		if (playbackPos >= playbackLength) {
			// If we're at the end of playback, either loop, or flag as done
			if (looping) {
				playbackPos = clip->getLoopPoint();
				if (playbackPos >= playbackLength) {
					looping = false;
					playbackPos = playbackLength;
					isPlaying = false;
				}
			} else {
				isPlaying = false;
			}
		}

		const size_t samplesAvailable = playbackLength - playbackPos;
		const size_t samplesRemaining = samplesRequested - samplesWritten;
		const size_t samplesToRead = std::min(samplesRemaining, samplesAvailable);

		if (samplesToRead > 0) {
			// We have some samples that we can read, so go ahead with reading them
			for (size_t srcChannel = 0; srcChannel < nChannels; ++srcChannel) {
				auto dst = gsl::span<AudioConfig::SampleFormat>(dstChannels[srcChannel].data() + samplesWritten, samplesToRead);
				size_t nCopied = clip->copyChannelData(srcChannel, playbackPos, samplesToRead, dst);
				Expects(nCopied <= samplesRequested * sizeof(AudioConfig::SampleFormat));
			}

			playbackPos += samplesToRead;
			samplesWritten += samplesToRead;
		} else {
			// Reached end of playback, pad with zeroes
			for (size_t srcChannel = 0; srcChannel < nChannels; ++srcChannel) {
				auto dstBuf = dstChannels[srcChannel];
				auto dst = dstBuf.data() + samplesWritten;
				memset(dst, 0, samplesRemaining * sizeof(AudioConfig::SampleFormat));
			}

			samplesWritten += samplesRemaining;
		}
	}

	return isPlaying;
}
