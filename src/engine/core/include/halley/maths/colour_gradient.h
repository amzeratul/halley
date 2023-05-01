#pragma once

#include "halley/data_structures/vector.h"
#include "colour.h"
#include "halley/bytes/config_node_serializer_base.h"
#include "halley/file_formats/image.h"

namespace Halley {
	class EntitySerializationContext;
	class ConfigNode;

	class ColourGradient {
    public:
        Vector<float> positions;
        Vector<Colour4f> colours;

        ColourGradient();
        ColourGradient(const ConfigNode& node);

        ConfigNode toConfigNode() const;

		void makeDefault();

        bool operator==(const ColourGradient& other) const;
        bool operator!=(const ColourGradient& other) const;

        void serialize(Serializer& s) const;
        void deserialize(Deserializer& s);

        Colour4f evaluate(float t) const;

		void render(Image& image);
	};

	template<>
	class ConfigNodeSerializer<ColourGradient> {
	public:
		ConfigNode serialize(const ColourGradient& gradient, const EntitySerializationContext& context);
		ColourGradient deserialize(const EntitySerializationContext& context, const ConfigNode& node);
	};
}
