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
		AudioVoice(std::shared_ptr<AudioSource> source, AudioPosition sourcePos, float gain, int group);
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
		
		void setId(size_t id);
		size_t getId() const;

		void setBehaviour(std::shared_ptr<AudioVoiceBehaviour> behaviour);
		
		int getGroup() const;

	private:
		bool playing = false;
		bool done = false;
		bool isFirstUpdate = true;
		uint8_t nChannels = 0;
		int group;
		size_t id = std::numeric_limits<size_t>::max();
    	float gain;
		float elapsedTime = 0.0f;

		std::shared_ptr<AudioSource> source;
		std::shared_ptr<AudioVoiceBehaviour> behaviour;
    	AudioPosition sourcePos;

		std::array<float, 16> channelMix;
		std::array<float, 16> prevChannelMix;

		void advancePlayback(size_t samples);
    };
}
