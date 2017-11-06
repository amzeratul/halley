#include "audio_source_clip.h"
#include "audio_clip.h"

using namespace Halley;


AudioSourceClip::AudioSourceClip(std::shared_ptr<const AudioClip> clip, bool looping)
	: clip(clip)
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
		playbackLength = clip->getLength();
		initialised = true;
	}

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
				auto src = clip->getChannelData(srcChannel, playbackPos, samplesToRead);
				auto dst = dstChannels[srcChannel].data() + samplesWritten;

				Expects(size_t(src.size_bytes()) <= samplesRequested * sizeof(AudioConfig::SampleFormat));
				if (src.size_bytes() > 0) {
					memcpy(dst, src.data(), src.size_bytes());
				}
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
