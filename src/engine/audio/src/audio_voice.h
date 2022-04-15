#pragma once
#include "audio_position.h"
#include "behaviours/audio_voice_behaviour.h"
#include "audio_buffer.h"
#include <limits>

#include "audio_fade.h"

namespace Halley {
	class AudioFilterResample;
	class AudioBufferPool;
	class AudioMixer;
	class AudioVoiceBehaviour;
	class AudioSource;

	class AudioVoice {
    public:
		AudioVoice(AudioEngine& engine, std::shared_ptr<AudioSource> source, float gain, float pitch, uint32_t delaySamples, uint8_t group);
		~AudioVoice();

		void start();

		void play(AudioFade fade);
		void stop(AudioFade fade);
		void pause(AudioFade fade);
		void resume(AudioFade fade);

		bool isPlaying() const;
		bool isReady() const;
		bool isDone() const;

		void setBaseGain(float gain);
		float getBaseGain() const;
		void setUserGain(float gain);
		float getUserGain() const;
		float& getDynamicGainRef();

		void setPitch(float pitch);

		size_t getNumberOfChannels() const;

		void update(gsl::span<const AudioChannelData> channels, const AudioPosition& sourcePos, const AudioListenerData& listener, float groupGain);
		void mixTo(size_t numSamples, gsl::span<AudioBuffer*> dst, AudioBufferPool& pool);
		
		void setIds(AudioEventId eventId, AudioObjectId audioObjectId = 0);
		AudioEventId getEventId() const;
		AudioObjectId getAudioObjectId() const;

		void addBehaviour(std::unique_ptr<AudioVoiceBehaviour> behaviour);
		
		uint8_t getGroup() const;

	private:
		enum class FadeEndBehaviour {
			None,
			Pause,
			Stop
		};
		
		AudioEngine& engine;
		
		AudioEventId eventId = std::numeric_limits<AudioEventId>::max();
		AudioObjectId audioObjectId = 0;
		uint8_t group = 0;
		uint8_t nChannels = 0;
		bool playing : 1;
		bool paused : 1;
		bool done : 1;
		bool isFirstUpdate : 1;
    	float baseGain = 1.0f;
		float dynamicGain = 1.0f;
		float userGain = 1.0f;
		float elapsedTime = 0.0f;
		uint32_t delaySamples = 0;

		AudioFader fader;
		FadeEndBehaviour fadeEnd = FadeEndBehaviour::None;

		std::shared_ptr<AudioSource> source;
		std::shared_ptr<AudioFilterResample> resample;
		std::unique_ptr<AudioVoiceBehaviour> behaviour;

		std::array<float, 16> channelMix;
		std::array<float, 16> prevChannelMix;

		void advancePlayback(size_t samples);
		void onFadeEnd();
    };
}
