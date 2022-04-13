#pragma once
#include "audio_emitter.h"

namespace Halley {
	class AudioFacade;

	class AudioEmitterHandleImpl final : public IAudioEmitterHandle {
    public:
        AudioEmitterHandleImpl(AudioFacade& facade, AudioEmitterId id);
        ~AudioEmitterHandleImpl() override;

        AudioEmitterId getId() const override;
        void detach() override;

    private:
        AudioFacade& facade;
        AudioEmitterId id;
        bool detached = false;
    };
}
