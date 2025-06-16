#pragma once

#include "audio_voice.h"
#include "halley/audio/audio_filter_biquad.h"

namespace Halley {
    class AudioRegion {
    public:
        struct Neighbour {
	        AudioRegionNeighbour props;
            AudioFilterBiquad filter;
            String name;
        };

        AudioRegion(AudioRegionId id, String name);

        AudioRegionId getId() const;
        const String& getName() const;

        void addNeighbour(AudioRegionNeighbour neighbour, String name);
        void removeNeighbour(AudioRegionId id);
        const Vector<Neighbour>& getNeighbours() const;
        Vector<Neighbour>& getNeighbours();

    	void markAsReadyToDestroy();
    	void clearRefCount();
        void incRefCount(AudioEngine& engine);
        bool shouldDestroy() const;

    	void setPrevGain(float gain);
        float getPrevGain() const;

    private:
        AudioRegionId id;

        bool readyToDestroy = false;
        int refCount = 0;
        float prevGain = 0;

        Vector<Neighbour> neighbours;
        String name;
    };
}
