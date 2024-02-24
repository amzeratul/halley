#pragma once

#include "circle.h"
#include "rect.h"
#include "vector2.h"
#include "vector3.h"

namespace Halley {
	class Random;
	class Polygon;

    class Triangle {
    public:
        Vector2f a, b, c;

        Triangle() = default;
        Triangle(Vector2f a, Vector2f b, Vector2f c)
	        : a(a), b(b), c(c)
        {}

        Rect4f getBounds() const;
        bool contains(Vector2f p) const;
        float getDistance(Vector2f p) const;

    	float getArea() const;
        float getPerimeter() const;

        Polygon toPolygon() const;

        Circle getCircumscribedCircle() const;

    	Vector2f getRandomPoint(Random& rng) const;

        Vector3f getBarycentricCoordinates(Vector2f p) const;

    	template<typename T>
        T interpolate(T valueAtA, T valueAtB, T valueAtC, Vector2f pointToSample) const
        {
            const auto coords = getBarycentricCoordinates(pointToSample);
            return coords.x * valueAtA + coords.y * valueAtB + coords.z * valueAtC;
        }

        Triangle operator+(Vector2f v) const;
        Triangle operator-(Vector2f v) const;
        Triangle operator*(Vector2f v) const;
        Triangle operator/(Vector2f v) const;
        Triangle operator*(float scalar) const;
        Triangle operator/(float scalar) const;

        Triangle& operator+=(Vector2f v);
        Triangle& operator-=(Vector2f v);
        Triangle& operator*=(Vector2f v);
        Triangle& operator/=(Vector2f v);
        Triangle& operator*=(float scalar);
        Triangle& operator/=(float scalar);

        bool operator==(const Triangle& other) const;
        bool operator!=(const Triangle& other) const;
    };
}
