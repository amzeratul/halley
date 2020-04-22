#pragma once
#include "halley/core/api/audio_api.h"
#include <functional>

namespace Halley
{
	class AudioFacade;
	class AudioVoice;

	class AudioHandleImpl final : public IAudioHandle
	{
	public:
		AudioHandleImpl(AudioFacade& facade, uint32_t id);
		
		void setGain(float gain) override;
		void setVolume(float volume) override;
		void setPosition(Vector2f pos) override;
		void setPan(float pan) override;
		void stop(float fadeTime) override;
		bool isPlaying() const override;
		void setBehaviour(std::unique_ptr<AudioVoiceBehaviour> behaviour) override;

	private:
		AudioFacade& facade;
		uint32_t handleId;

		void enqueue(std::function<void(AudioVoice& src)> f);
	};
}
