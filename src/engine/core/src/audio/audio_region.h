#pragma once

#include "audio_voice.h"

namespace Halley {
    class AudioRegion {
    public:
        AudioRegion(AudioRegionId id);

        AudioRegionId getId() const;

        void addNeighbour(AudioRegionId id, float attenuation, float lowPassHz);
        void removeNeighbour(AudioRegionId id);

    private:
        AudioRegionId id;
    };
}
