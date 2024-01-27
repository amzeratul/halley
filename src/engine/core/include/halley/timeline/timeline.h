#pragma once

#include "timeline_sequence.h"
#include "halley/bytes/config_node_serializer_base.h"

namespace Halley {
    class Timeline {
    public:
        Timeline() = default;
        Timeline(const ConfigNode& node);

		void load(const ConfigNode& node);
        ConfigNode toConfigNode() const;

    private:
        Vector<TimelineSequence> sequences;
    };

	class Resources;
	template<>
	class ConfigNodeSerializer<Timeline> {
	public:
		ConfigNode serialize(const Timeline& timeline, const EntitySerializationContext& context);
		Timeline deserialize(const EntitySerializationContext& context, const ConfigNode& node);
		void deserialize(const EntitySerializationContext& context, const ConfigNode& node, Timeline& target);
	};
}
