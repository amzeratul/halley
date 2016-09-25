#pragma once
#include "halley/core/api/audio_api.h"

namespace Halley
{
	class AudioFacade;

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
	};
}
