#pragma once
#include "audio_emitter.h"

namespace Halley {
	class AudioFacade;

	class AudioEmitterHandleImpl final : public IAudioEmitterHandle {
    public:
        AudioEmitterHandleImpl(AudioFacade& facade, AudioEmitterId id, AudioPosition position, bool owningHandle);
        ~AudioEmitterHandleImpl() override;

        AudioEmitterId getId() const override;
        void detach() override;

        void setSwitch(String switchId, String value) override;
		void setVariable(String variableId, float value) override;
        void setPosition(AudioPosition position) override;
        void setGain(float gain) override;
        void setRegion(AudioRegionId regionId) override;

        AudioPosition getPosition() const override;

    private:
        AudioFacade& facade;
        AudioEmitterId id;
        AudioPosition position;
        bool detached = false;
        bool owning = true;
    };
}
