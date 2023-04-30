#pragma once

#include "halley/maths/vector2.h"
#include "halley/data_structures/vector.h"
#include "tween.h"

namespace Halley {
    class InterpolationCurve {
    public:
        Vector<Vector2f> points;
        Vector<TweenCurve> tweens;
        float scale = 1.0f;

        float evaluate(float t) const;
        float evaluateRaw(float t) const;
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
}
