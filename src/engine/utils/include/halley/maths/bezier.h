#pragma once
#include "vector2.h"

namespace Halley {
    class BezierQuadratic {
    public:
    	Vector2f p0;
    	Vector2f p1;
    	Vector2f p2;

    	BezierQuadratic() = default;
    	BezierQuadratic(Vector2f p0, Vector2f p1, Vector2f p2)
    		: p0(p0), p1(p1), p2(p2)
    	{}

    	Vector2f evaluate(float t) const
    	{
    		const float u = 1 - t;
    		return u * u * p0 +
				2 * u * t * p1 +
				t * t * p2;
    	}

    	Vector2f getStartPoint() const { return p0; }
        Vector2f getEndPoint() const { return p2; }

        std::pair<BezierQuadratic, BezierQuadratic> splitAt(float t) const;
        bool isFlat(float tolerance) const;
    	Vector<Vector2f> toLineSegments() const;
    };

    class BezierCubic {
    public:
        Vector2f p0;
    	Vector2f p1;
    	Vector2f p2;
    	Vector2f p3;

    	BezierCubic() = default;
    	BezierCubic(Vector2f p0, Vector2f p1, Vector2f p2, Vector2f p3)
    		: p0(p0), p1(p1), p2(p2), p3(p3)
    	{}

    	Vector2f evaluate(float t) const
    	{
    		const float u = 1 - t;
    		return u * u * u * p0 +
				3 * u * u * t * p1 +
				3 * u * t * t * p2 +
				t * t * t * p3;
    	}

        std::pair<BezierCubic, BezierCubic> splitAt(float t) const;
        bool isFlat(float tolerance) const;
    	Vector<Vector2f> toLineSegments() const;

        Vector2f getStartPoint() const { return p0; }
        Vector2f getEndPoint() const { return p3; }

    	BezierCubic operator+(Vector2f v) const
    	{
    		return BezierCubic{ p0 + v, p1 + v, p2 + v, p3 + v };
    	}

    	BezierCubic operator-(Vector2f v) const
    	{
    		return BezierCubic{ p0 - v, p1 - v, p2 - v, p3 - v };
    	}
    };
}
