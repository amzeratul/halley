#pragma once

#include "audio_voice.h"

namespace Halley {
    class AudioRegion {
    public:
        AudioRegion(AudioRegionId id);

        AudioRegionId getId() const;

        void addNeighbour(AudioRegionId id, float attenuation, float lowPassHz);
        void removeNeighbour(AudioRegionId id);

    	void markAsReadyToDestroy();
    	void clearRefCount();
        void incRefCount();
        bool shouldDestroy() const;

    	void setPrevGain(float gain);
        float getPrevGain() const;

    private:
        AudioRegionId id;

        bool readyToDestroy = false;
        int refCount = 0;
        float prevGain = 0;
    };
}
