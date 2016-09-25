#pragma once
#include "halley/core/api/audio_api.h"
#include <functional>

namespace Halley
{
	class AudioFacade;
	class AudioSource;

	class AudioHandleImpl : public IAudioHandle
	{
	public:
		AudioHandleImpl(AudioFacade& facade, size_t id);
		
		void setGain(float gain) override;
		void setPosition(Vector2f pos) override;
		void setPan(float pan) override;
		void stop() override;
		bool isPlaying() const override;

	private:
		AudioFacade& facade;
		size_t handleId;

		void enqueue(std::function<void(AudioSource& src)> f);
	};
}
