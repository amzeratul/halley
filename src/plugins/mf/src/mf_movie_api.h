#pragma once

#include "halley/core/api/halley_api_internal.h"

namespace Halley {
    class MFMovieAPI : public MovieAPIInternal {
    public:
	    explicit MFMovieAPI(SystemAPI& system);
        void init() override;
        void deInit() override;

	    std::shared_ptr<MoviePlayer> makePlayer(VideoAPI& video, AudioAPI& audio, std::shared_ptr<ResourceDataStream> data) override;

    private:
		SystemAPI& system;
    };
}
