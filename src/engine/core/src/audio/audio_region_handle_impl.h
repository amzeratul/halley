#pragma once
#include "audio_emitter.h"
#include "audio_region.h"

namespace Halley {
	class AudioFacade;

	class AudioRegionHandleImpl final : public IAudioRegionHandle {
    public:
        AudioRegionHandleImpl(AudioFacade& facade, AudioRegionId id);
        ~AudioRegionHandleImpl() override;

        AudioRegionId getId() const override;

        void addNeighbour(AudioRegionNeighbour neighbour) override;
        void removeNeighbour(AudioRegionId id) override;

    private:
        AudioFacade& facade;
        AudioRegionId id;

		void enqueue(std::function<void(AudioRegion& region)> f);
    };
}
