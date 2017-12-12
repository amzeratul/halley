#pragma once
#include "audio_position.h"
#include "audio_emitter_behaviour.h"
#include "audio_buffer.h"
#include <boost/container/allocator_traits.hpp>
#include <boost/predef/detail/_exception.h>
#include <boost/predef/detail/_exception.h>
#include <limits>

namespace Halley {
	class AudioBufferPool;
	class AudioMixer;
	class AudioEmitterBehaviour;
	class AudioSource;

	class AudioEmitter {
    public:
		AudioEmitter(std::shared_ptr<AudioSource> source, AudioPosition sourcePos, float gain);
		~AudioEmitter();

		void start();
		void stop();

		bool isPlaying() const;
		bool isReady() const;
		bool isDone() const;

		void setGain(float gain);
		void setAudioSourcePosition(AudioPosition sourcePos);

		float getGain() const;
		size_t getNumberOfChannels() const;

		void update(gsl::span<const AudioChannelData> channels, const AudioListenerData& listener);
		void mixTo(size_t numSamples, gsl::span<AudioBuffer*> dst, AudioMixer& mixer, AudioBufferPool& pool);
		
		void setId(size_t id);
		size_t getId() const;

		void setBehaviour(std::shared_ptr<AudioEmitterBehaviour> behaviour);

	private:
		std::shared_ptr<AudioSource> source;
		std::shared_ptr<AudioEmitterBehaviour> behaviour;

    	AudioPosition sourcePos;
		bool playing = false;
		bool done = false;
		bool isFirstUpdate = true;
    	float gain;
		float elapsedTime = 0.0f;

		size_t nChannels = 0;
		std::array<float, 16> channelMix;
		std::array<float, 16> prevChannelMix;

		size_t id = std::numeric_limits<size_t>::max();

		void advancePlayback(size_t samples);
    };
}
