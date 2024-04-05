#pragma once

#include "halley/maths/vector2.h"
#include "halley/data_structures/vector.h"
#include "tween.h"
#include "halley/bytes/config_node_serializer_base.h"

namespace Halley {
	class EntitySerializationContext;
	class ConfigNode;

	class InterpolationCurve {
    public:
        Vector<Vector2f> points;
        Vector<TweenCurve> tweens;
        float scale = 1.0f;
        float baseline = 0.0f;

        InterpolationCurve();
        InterpolationCurve(const ConfigNode& node, bool startFromZero = true);

        ConfigNode toConfigNode() const;

		void makeDefault(bool startFromZero = true);

        bool operator==(const InterpolationCurve& other) const;
        bool operator!=(const InterpolationCurve& other) const;

        void serialize(Serializer& s) const;
        void deserialize(Deserializer& s);

        float evaluate(float t) const;
        float evaluateRaw(float t) const;

		bool isTrivial() const;
        float getMaxAbsValue() const;
	};

    class PrecomputedInterpolationCurve {
    public:
        PrecomputedInterpolationCurve() = default;
        PrecomputedInterpolationCurve(const InterpolationCurve& src);

        float evaluate(float t) const;

    private:
        constexpr static size_t nElements = 64;
        alignas(64) std::array<uint8_t, nElements> elements;
        float scale = 1.0f;
    };

	template<>
	class ConfigNodeSerializer<InterpolationCurve> {
	public:
		ConfigNode serialize(const InterpolationCurve& curve, const EntitySerializationContext& context);
		InterpolationCurve deserialize(const EntitySerializationContext& context, const ConfigNode& node);
	};
}
