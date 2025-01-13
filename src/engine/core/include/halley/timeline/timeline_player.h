#pragma once

#include "timeline.h"

namespace Halley {
    class TimelinePlayer {
    public:
        void serialize(Serializer& serializer) const;
        void deserialize(Deserializer& deserializer);
    };
}
