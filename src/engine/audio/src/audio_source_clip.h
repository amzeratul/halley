#pragma once
#include "audio_source.h"

namespace Halley
{
	class AudioSourceClip : public AudioSource
	{
	public:
		AudioSourceClip(std::shared_ptr<const IAudioClip> clip, bool looping, int64_t delaySamples);

		uint8_t getNumberOfChannels() const override;
		bool getAudioData(size_t numSamples, AudioSourceData& dst) override;
		bool isReady() const override;

	private:
		const std::shared_ptr<const IAudioClip> clip;
		
		int64_t playbackPos = 0;

		bool initialised = false;
		bool looping;
	};
}
