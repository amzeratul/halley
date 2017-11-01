#pragma once
#include "audio_clip.h"
#include "audio_source_position.h"
#include "audio_source_behaviour.h"
#include "audio_buffer.h"
#include <boost/container/allocator_traits.hpp>
#include <boost/predef/detail/_exception.h>
#include <boost/predef/detail/_exception.h>

namespace Halley {
	class AudioBufferPool;
	class AudioMixer;
	class AudioSourceBehaviour;

	class AudioSource {
    public:
		AudioSource(std::shared_ptr<const AudioClip> clip, AudioSourcePosition sourcePos, float gain, bool loop);
		~AudioSource();

		void start();
		void stop();

		bool isPlaying() const;
		bool isReady() const;
		bool isDone() const;

		void setGain(float gain);
		void setAudioSourcePosition(AudioSourcePosition sourcePos);

		float getGain() const;
		size_t getNumberOfChannels() const;

		void update(gsl::span<const AudioChannelData> channels, const AudioListenerData& listener);
		void mixTo(gsl::span<AudioBuffer> dst, AudioMixer& mixer, AudioBufferPool& pool);
		
		void setId(size_t id);
		size_t getId() const;

		void setBehaviour(std::shared_ptr<AudioSourceBehaviour> behaviour);

	private:
		std::shared_ptr<const AudioClip> clip;
		std::shared_ptr<AudioSourceBehaviour> behaviour;

		size_t playbackPos = 0;
		size_t playbackLength = 0;

    	AudioSourcePosition sourcePos;
		bool playing = false;
		bool done = false;
		bool looping = false;
		bool isFirstUpdate = true;
    	float gain;
		float elapsedTime = 0.0f;

		size_t nChannels = 0;
		std::array<float, 16> channelMix;
		std::array<float, 16> prevChannelMix;

		size_t id = -1;

		void readSourceToBuffer(size_t srcChannel, gsl::span<AudioSamplePack> dst) const;
		void advancePlayback(size_t samples);
    };
}
