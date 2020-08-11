#pragma once
#include "audio_position.h"
#include "audio_voice_behaviour.h"
#include "audio_buffer.h"
#include <limits>

namespace Halley {
	class AudioBufferPool;
	class AudioMixer;
	class AudioVoiceBehaviour;
	class AudioSource;

	class AudioVoice {
    public:
		AudioVoice(std::shared_ptr<AudioSource> source, AudioPosition sourcePos, float gain, uint8_t group);
		~AudioVoice();

		void start();
		void stop();

		bool isPlaying() const;
		bool isReady() const;
		bool isDone() const;

		void setGain(float gain);
		void setAudioSourcePosition(Vector3f position);
		void setAudioSourcePosition(AudioPosition sourcePos);

		float getGain() const;
		size_t getNumberOfChannels() const;

		void update(gsl::span<const AudioChannelData> channels, const AudioListenerData& listener, float groupGain);
		void mixTo(size_t numSamples, gsl::span<AudioBuffer*> dst, AudioMixer& mixer, AudioBufferPool& pool);
		
		void setId(uint32_t id);
		uint32_t getId() const;

		void setBehaviour(std::unique_ptr<AudioVoiceBehaviour> behaviour);
		
		uint8_t getGroup() const;

	private:
		uint32_t id = std::numeric_limits<uint32_t>::max();
		uint8_t group = 0;
		uint8_t nChannels = 0;
		bool playing = false;
		bool done = false;
		bool isFirstUpdate = true;
    	float gain;
		float elapsedTime = 0.0f;

		std::shared_ptr<AudioSource> source;
		std::unique_ptr<AudioVoiceBehaviour> behaviour;
    	AudioPosition sourcePos;

		std::array<float, 16> channelMix;
		std::array<float, 16> prevChannelMix;

		void advancePlayback(size_t samples);
    };
}
