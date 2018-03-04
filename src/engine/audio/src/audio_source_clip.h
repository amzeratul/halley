#pragma once
#include "audio_source.h"

namespace Halley
{
	class AudioSourceClip : public AudioSource
	{
	public:
		AudioSourceClip(std::shared_ptr<const IAudioClip> clip, bool looping);

		size_t getNumberOfChannels() const override;
		bool getAudioData(size_t numSamples, AudioSourceData& dst) override;
		bool isReady() const override;

	private:
		const std::shared_ptr<const IAudioClip> clip;
		
		size_t playbackPos = 0;
		size_t playbackLength = 0;

		bool initialised = false;
		bool looping;
	};
}
