#pragma once
#include "audio_clip.h"
#include "audio_source_position.h"

namespace Halley {
	class AudioMixer;

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
		void mixToBuffer(size_t srcChannel, size_t dstChannel, gsl::span<AudioSamplePack> tmp, gsl::span<AudioSamplePack> out, AudioMixer& mixer);
		void advancePlayback(size_t samples);
		
    private:
		std::shared_ptr<AudioClip> clip;

		size_t playbackPos = 0;
		size_t playbackLength = 0;

    	AudioSourcePosition sourcePos;
		bool playing = false;
		bool done = false;
    	float gain;

		std::array<float, 16> channelMix;
		std::array<float, 16> prevChannelMix;

    	void readSourceToBuffer(size_t srcChannel, gsl::span<AudioSamplePack> dst) const;
	    bool canDoDirectRead(size_t size) const;
    };
}
