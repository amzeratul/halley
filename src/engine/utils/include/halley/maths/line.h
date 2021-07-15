/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include "vector2.h"
#include <optional>

#include "range.h"

namespace Halley {
	class Line {
	public:
		constexpr Line() = default;
		constexpr Line(Vector2f origin, Vector2f dir)
			: origin(origin)
			, dir(dir.normalized())
		{}
		
		Vector2f origin;
		Vector2f dir;

		float getDistance(Vector2f point) const
		{
			return (point - origin).neutralize(dir).length();
		}

		Vector2f getClosestPoint(Vector2f point) const
		{
			return (point - origin).projection(dir) + origin;
		}

		std::optional<Vector2f> intersection(const Line& other) const
		{
			const auto& a = origin;
			const auto& b = dir;
			const auto& c = other.origin;
			const auto& d = other.dir;
			const float divisor = b.x * d.y - b.y * d.x;
			if (std::abs(divisor) < 0.000001f) {
				// Parallel lines
				return {};
			}
			const float t = (d.x * (a.y - c.y) + d.y * (c.x - a.x)) / divisor;
			return a + t * b;
		}

		static std::vector<Vector2i> generateLine(Vector2i p0, Vector2i p1);
		static void doLine(Vector2i p0, Vector2i p1, std::function<void(Vector2i)> callback);
	};

	class LineSegment {
	public:
		constexpr LineSegment() = default;
		constexpr LineSegment(Vector2f a, Vector2f b)
			: a(a), b(b)
		{}

		Vector2f a;
		Vector2f b;

		constexpr Vector2f getClosestPoint(Vector2f point) const
		{
			const float len = (b - a).length();
			const Vector2f dir = (b - a) * (1.0f / len);
			const float x = (point - a).dot(dir); // position along the A-B segment
			return a + dir * clamp(x, 0.0f, len);
		}

		std::optional<Vector2f> intersection(const LineSegment& other, const float epsilon = 0) const
		{
			const float len = (this->b - this->a).length();
			const float otherLen = (other.b - other.a).length();
			const auto a = this->a;
			const auto b = (this->b - this->a) / len;
			const auto c = other.a;
			const auto d = (other.b - other.a) / otherLen;
			const float divisor = b.x * d.y - b.y * d.x;
			if (std::abs(divisor) < 0.000001f) {
				// Parallel lines
				return {};
			}
			const float t = (d.x * (a.y - c.y) + d.y * (c.x - a.x)) / divisor;
			const float u = -(b.x * (c.y - a.y) + b.y * (a.x - c.x)) / divisor;
			
			if (t < -epsilon || t > len + epsilon || u < -epsilon || u > otherLen + epsilon) {
				// Out of edges
				return {};
			}
			return a + t * b;
		}

		std::optional<Vector2f> intersection(const Line& other) const
		{
			const float len = (this->b - this->a).length();
			const auto a = this->a;
			const auto b = (this->b - this->a) / len;
			const auto c = other.origin;
			const auto d = other.dir;
			const float divisor = b.x * d.y - b.y * d.x;
			if (std::abs(divisor) < 0.000001f) {
				// Parallel lines
				return {};
			}
			const float t = (d.x * (a.y - c.y) + d.y * (c.x - a.x)) / divisor;
			const float u = -(b.x * (c.y - a.y) + b.y * (a.x - c.x)) / divisor;
			
			return a + t * b;
		}

		bool sharesVertexWith(const LineSegment& other) const
		{
			const float epsilon = 0.000001f;
			return a.epsilonEquals(other.a, epsilon)
				|| a.epsilonEquals(other.b, epsilon)
				|| b.epsilonEquals(other.a, epsilon)
				|| b.epsilonEquals(other.b, epsilon);
		}

		bool contains(Vector2f point, const float epsilon = 0.001) const
		{
			return (getClosestPoint(point) - point).length() < epsilon;
		}

		bool hasEndpoint(Vector2f point) const
		{
			const float epsilon = 0.000001f;
			return a.epsilonEquals(point, epsilon)
				|| b.epsilonEquals(point, epsilon);
		}

		bool epsilonEquals(const LineSegment& other, float epsilon) const
		{
			return (a.epsilonEquals(other.a, epsilon) && b.epsilonEquals(other.b, epsilon))
				|| (a.epsilonEquals(other.b, epsilon) && b.epsilonEquals(other.a, epsilon));
		}

		Range<float> project(const Vector2f& axis) const
		{
			const float dotA = axis.dot(a);
			const float dotB = axis.dot(b);

			return Range<float>(dotA, dotB);
		}

		bool operator==(const LineSegment& other) const
		{
			return a == other.a && b == other.b;
		}

		bool operator!=(const LineSegment& other) const
		{
			return a != other.a || b != other.b;
		}
	};
}
