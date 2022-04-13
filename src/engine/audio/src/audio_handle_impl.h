#pragma once
#include "halley/core/api/audio_api.h"
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
		void stop(float fadeTime) override;
		bool isPlaying() const override;
		void addBehaviour(std::unique_ptr<AudioVoiceBehaviour> behaviour) override;

	private:
		AudioFacade& facade;
		AudioEventId eventId;
		AudioEmitterId emitterId;
		float gain = 1.0f;

		void enqueue(std::function<void(AudioEmitter& src)> f);
		void enqueueForVoices(std::function<void(AudioVoice& src)> f);
	};
}
