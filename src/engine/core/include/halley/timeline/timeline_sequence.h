#pragma once

#include "halley/data_structures/config_node.h"

namespace Halley {
    class TimelineSequence {
    public:
        TimelineSequence() = default;
        TimelineSequence(const ConfigNode& node);

        ConfigNode toConfigNode() const;
    };
}
