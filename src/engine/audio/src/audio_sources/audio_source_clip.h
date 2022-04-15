#pragma once
#include "audio_source.h"

namespace Halley
{
	class AudioSourceClip final : public AudioSource
	{
	public:
		AudioSourceClip(std::shared_ptr<const IAudioClip> clip, bool looping, int64_t delaySamples, int64_t loopStart, int64_t loopEnd);

		uint8_t getNumberOfChannels() const override;
		bool getAudioData(size_t numSamples, AudioMultiChannelSamples dst) override;
		bool isReady() const override;

	private:
		const std::shared_ptr<const IAudioClip> clip;
		
		int64_t playbackPos = 0;
		int64_t loopStart = 0;
		int64_t loopEnd = 0;

		bool initialised = false;
		bool looping;
	};
}
