#pragma once
#include "audio_clip.h"
#include "audio_source_position.h"

namespace Halley {
    class AudioSource {
    public:
		AudioSource(std::shared_ptr<AudioClip> clip, AudioSourcePosition sourcePos, float gain);

		void start();
		void stop();

		bool isPlaying() const;
		bool isReady() const;
		bool isDone() const;

		size_t getNumberOfChannels() const;

		void update(gsl::span<const AudioChannelData> channels);
		void mixToBuffer(size_t srcChannel, size_t dstChannel, gsl::span<AudioSamplePack> tmp, gsl::span<AudioSamplePack> out);
		void advancePlayback(size_t samples);
		
    private:
		std::shared_ptr<AudioClip> clip;
		AudioSourcePosition sourcePos;
		float gain;

		size_t playbackPos = 0;
		size_t playbackLength = 0;
		bool playing = false;
		bool done = false;

		std::array<float, 8> prevChannelMix;
		std::array<float, 8> channelMix;

		void readSourceToBuffer(size_t srcChannel, gsl::span<AudioSamplePack> dst) const;
    };
}
