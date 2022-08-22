#pragma once
#include "audio_source.h"
#include "halley/maths/range.h"

namespace Halley
{
	class AudioSourceClip final : public AudioSource
	{
	public:
		AudioSourceClip(AudioEngine& engine, std::shared_ptr<const IAudioClip> clip, bool looping, float gain, int64_t loopStart, int64_t loopEnd, bool randomiseStart);

		uint8_t getNumberOfChannels() const override;
		bool getAudioData(size_t numSamples, AudioMultiChannelSamples dst) override;
		bool isReady() const override;
		size_t getSamplesLeft() const override;
		void restart() override;

	private:
		AudioEngine& engine;
		const std::shared_ptr<const IAudioClip> clip;

		struct PlayStream {
			size_t playbackPos = 0;
			size_t endPos = 0;
			bool active = false;
			bool loop = false;
			bool kickOffSecondStream = false;
		};
		std::array<PlayStream, 2> streams;

		int64_t loopStart = 0;
		int64_t loopEnd = 0;
		float gain = 1;
		float prevGain = 1;

		bool initialised = false;
		bool looping = false;
		bool randomiseStart = false;
	};
}
