#pragma once
#include "halley/api/audio_api.h"
#include <functional>

namespace Halley
{
	class AudioFacade;
	class AudioEmitter;
	class AudioVoice;

	class AudioHandleImpl final : public IAudioHandle
	{
	public:
		AudioHandleImpl(AudioFacade& facade, AudioEventId eventId, AudioEmitterId emitterId);

		void setGain(float gain) override;
		void setVolume(float volume) override;
		void setPosition(Vector2f pos) override;
		void setPan(float pan) override;

		void play(const AudioFade& audioFade) override;
		void stop(const AudioFade& audioFade) override;
		void pause(const AudioFade& audioFade) override;
		void resume(const AudioFade& audioFade) override;
		
		void stop(float fadeTime) override;
		bool isPlaying() const override;

	private:
		AudioFacade& facade;
		AudioEventId eventId;
		AudioEmitterId emitterId;
		float gain = 1.0f;

		void enqueue(std::function<void(AudioEmitter& src)> f);
		void enqueueForVoices(std::function<void(AudioVoice& src)> f);
	};
}
