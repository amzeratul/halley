#pragma once
#include "audio_position.h"
#include "behaviours/audio_voice_behaviour.h"
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

		void setBaseGain(float gain);
		float getBaseGain() const;
		void setUserGain(float gain);
		float getUserGain() const;
		float& getDynamicGainRef();

		void setAudioSourcePosition(Vector3f position);
		void setAudioSourcePosition(AudioPosition sourcePos);

		size_t getNumberOfChannels() const;

		void update(gsl::span<const AudioChannelData> channels, const AudioListenerData& listener, float groupGain);
		void mixTo(size_t numSamples, gsl::span<AudioBuffer*> dst, AudioMixer& mixer, AudioBufferPool& pool);
		
		void setId(uint32_t id);
		uint32_t getId() const;

		void addBehaviour(std::unique_ptr<AudioVoiceBehaviour> behaviour);
		
		uint8_t getGroup() const;

	private:
		uint32_t id = std::numeric_limits<uint32_t>::max();
		uint8_t group = 0;
		uint8_t nChannels = 0;
		bool playing : 1;
		bool done : 1;
		bool isFirstUpdate : 1;
    	float baseGain = 1.0f;
		float dynamicGain = 1.0f;
		float userGain = 1.0f;
		float elapsedTime = 0.0f;

		std::shared_ptr<AudioSource> source;
		std::unique_ptr<AudioVoiceBehaviour> behaviour;
    	AudioPosition sourcePos;

		std::array<float, 16> channelMix;
		std::array<float, 16> prevChannelMix;

		void advancePlayback(size_t samples);
    };
}
