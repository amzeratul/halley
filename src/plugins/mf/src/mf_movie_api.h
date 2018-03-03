#pragma once

#include "halley/core/api/halley_api_internal.h"

namespace Halley {
    class MFMovieAPI : public MovieAPIInternal {
    public:
        void init() override;
        void deInit() override;
    };
}
