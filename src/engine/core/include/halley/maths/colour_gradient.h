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
		ColourGradient(float fadeInEnd, float fadeOutStart);

        ConfigNode toConfigNode() const;

		void makeDefault();

        bool operator==(const ColourGradient& other) const;
        bool operator!=(const ColourGradient& other) const;

        void serialize(Serializer& s) const;
        void deserialize(Deserializer& s);

		Colour4f evaluateSource(float t) const;
		Colour4f evaluatePrecomputed(float t) const;

        void markDirty();
		void render(Image& image);

		void clear();
		void add(Colour4f col, float position);

		bool isTrivial() const;

    private:
		constexpr static int precomputedSize = 128;

		mutable Vector<Colour4f> precomputed;
		mutable bool dirty = true;

		void precompute() const;
	};

	template<>
	class ConfigNodeSerializer<ColourGradient> {
	public:
		ConfigNode serialize(const ColourGradient& gradient, const EntitySerializationContext& context);
		ColourGradient deserialize(const EntitySerializationContext& context, const ConfigNode& node);
	};
}
