#pragma once
#include "halley/core/api/audio_api.h"
#include <functional>

namespace Halley
{
	class AudioFacade;
	class AudioEmitter;

	class AudioHandleImpl : public IAudioHandle
	{
	public:
		AudioHandleImpl(AudioFacade& facade, size_t id);
		
		void setGain(float gain) override;
		void setVolume(float volume) override;
		void setPosition(Vector2f pos) override;
		void setPan(float pan) override;
		void stop(float fadeTime) override;
		bool isPlaying() const override;
		void setBehaviour(std::unique_ptr<AudioEmitterBehaviour> behaviour) override;

	private:
		AudioFacade& facade;
		size_t handleId;

		void enqueue(std::function<void(AudioEmitter& src)> f);
	};
}
