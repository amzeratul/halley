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
#include "range.h"
#include <optional>

#include "vector4.h"

namespace Halley {
	//////////////////////////////
	// Vector2D class declaration
	template <typename T=float>
	class alignas(sizeof(T)*4) Rect2D {
		Vector2D<T> p1, p2;

	public:
		Rect2D() = default;
		Rect2D(const Rect2D<T>& r) = default;
		Rect2D(Rect2D<T>&& r) noexcept = default;

		Rect2D<T>& operator=(const Rect2D<T>& o) = default;
		Rect2D<T>& operator=(Rect2D<T>&& o) noexcept = default;

		template<typename U>
		constexpr explicit Rect2D(Rect2D<U> r)
		{
			set(T(r.getX()), T(r.getY()), T(r.getWidth()), T(r.getHeight()));
		}

		constexpr Rect2D(Vector2D<T> point1, Vector2D<T> point2)
		{
			set(point1, point2);
		}

		constexpr Rect2D(Vector2D<T> origin, T width, T height)
		{
			set(origin, Vector2D<T>(origin.x+width, origin.y+height));
		}

		constexpr Rect2D(T x, T y, T width, T height)
		{
			set(x, y, width, height);
		}

		constexpr void set(T x, T y, T width, T height)
		{
			set(Vector2D<T>(x, y), Vector2D<T>(x+width, y+height));
		}

		constexpr void set(Vector2D<T> point1, Vector2D<T> point2)
		{
			p1.x = std::min(point1.x, point2.x);
			p1.y = std::min(point1.y, point2.y);
			p2.x = std::max(point1.x, point2.x);
			p2.y = std::max(point1.y, point2.y);
		}

		constexpr void setPos(Vector2D<T> pos)
		{
			auto sz = p2 - p1;
			p1 = pos;
			p2 = pos + sz;
		}

		constexpr void setX(T x)
		{
			float dx = x - p1.x;
			p1.x += dx;
			p2.x += dx;
		}

		constexpr void setY(T y)
		{
			float dy = y - p1.y;
			p1.y += dy;
			p2.y += dy;
		}

		constexpr void setWidth(T w)
		{
			p2.x = p1.x + w;
		}

		constexpr void setHeight(T h)
		{
			p2.y = p1.y + h;
		}

		constexpr void setSize(Vector2D<T> size)
		{
			p2 = p1 + size;
		}

		constexpr void setLeft(T value)
		{
			p1.x = value;
		}

		constexpr void setRight(T value)
		{
			p2.x = value;
		}

		constexpr void setTop(T value)
		{
			p1.y = value;
		}

		constexpr void setBottom(T value)
		{
			p2.y = value;
		}

		constexpr Rect2D<T> shrink(T amount) const
		{
			auto offset = Vector2D<T>(amount, amount);
			return Rect2D(p1 + offset, p2 - offset);
		}

		constexpr Rect2D<T> grow(T amount) const
		{
			auto offset = Vector2D<T>(amount, amount);
			return Rect2D(p1 - offset, p2 + offset);
		}

		constexpr Rect2D<T> grow(T left, T top, T right, T bottom) const
		{
			return Rect2D(p1 - Vector2D<T>(left, top), p2 + Vector2D<T>(right, bottom));
		}

		constexpr Rect2D<T> grow(Vector4D<T> v) const
		{
			return Rect2D(p1 - v.xy(), p2 + v.zw());
		}

		constexpr Vector2D<T> getTopLeft() const { return p1; }
		constexpr Vector2D<T> getBottomRight() const { return p2; }
		constexpr Vector2D<T> getBottomLeft() const { return Vector2D<T>(p1.x, p2.y); }
		constexpr Vector2D<T> getTopRight() const { return Vector2D<T>(p2.x, p1.y); }
		constexpr Vector2D<T> getSize() const { return p2 - p1; }

		constexpr Vector2D<T>& getP1() { return p1; }
		constexpr Vector2D<T>& getP2() { return p2; }

		constexpr T getWidth() const { return p2.x - p1.x; }
		constexpr T getHeight() const { return p2.y - p1.y; }
		constexpr T getX() const { return p1.x; }
		constexpr T getY() const { return p1.y; }

		constexpr T getLeft() const { return p1.x; }
		constexpr T getRight() const { return p2.x; }
		constexpr T getTop() const { return p1.y; }
		constexpr T getBottom() const { return p2.y; }

		[[nodiscard]] constexpr bool contains(Vector2D<T> p) const
		{
			return (p.x >= p1.x && p.x < p2.x && p.y >= p1.y && p.y < p2.y);
		}

		[[nodiscard]] constexpr Vector2D<T> getClosestPoint(Vector2D<T> p) const
		{
			p.x = clamp(p.x, p1.x, p2.x - 1);
			p.y = clamp(p.y, p1.y, p2.y - 1);
			return p;
		}

		[[nodiscard]] constexpr Vector2D<T> wrapInside(Vector2D<T> p) const
		{
			Vector2D<T> size = getSize();
			return ((((p-p1) % size) + size) % size) + p1;
		}

		[[nodiscard]] constexpr Rect2D<T> intersection(const Rect2D<T>& p) const
		{
			Range<T> x0(p1.x, p2.x);
			Range<T> x1(p.p1.x, p.p2.x);
			Range<T> x = x0.getOverlap(x1);
			Range<T> y0(p1.y, p2.y);
			Range<T> y1(p.p1.y, p.p2.y);
			Range<T> y = y0.getOverlap(y1);
			return Rect2D<T>(Vector2D<T>(x.start, y.start), Vector2D<T>(x.end, y.end));
		}

		[[nodiscard]] constexpr Rect2D<T> merge(const Rect2D<T>& other) const
		{
			return Rect2D<T>(Vector2D<T>::min(p1, other.p1), Vector2D<T>::max(p2, other.p2));
		}

		Range<T> getHorizontal() const
		{
			return Range<T>(p1.x, p2.x);
		}

		Range<T> getVertical() const
		{
			return Range<T>(p1.y, p2.y);
		}

		Range<T> getRange(size_t i) const
		{
			return Range<T>(p1[i], p2[i]);
		}

		constexpr Rect2D<T> operator+(Vector2D<T> v) const
		{
			return Rect2D<T>(p1 + v, p2 + v);
		}

		constexpr Rect2D<T> operator+(Rect2D<T> v) const
		{
			return Rect2D<T>(p1 + v.p1, p2 + v.p2);
		}

		constexpr Rect2D<T> operator-(Vector2D<T> v) const
		{
			return Rect2D<T>(p1 - v, p2 - v);
		}

		constexpr Rect2D<T> operator-(Rect2D<T> v) const
		{
			return Rect2D<T>(p1 - v.p1, p2 - v.p2);
		}

		constexpr Rect2D<T>& operator+=(Vector2D<T> v)
		{
			p1 += v;
			p2 += v;
			return *this;
		}

		constexpr Rect2D<T>& operator-=(Vector2D<T> v)
		{
			p1 -= v;
			p2 -= v;
			return *this;
		}

		constexpr bool overlaps(Rect2D<T> other) const
		{
			return !(p2.x <= other.p1.x || other.p2.x <= p1.x || p2.y <= other.p1.y || other.p2.y <= p1.y);
		}

		constexpr Vector2D<T> getCenter() const
		{
			return (p1+p2)/2;
		}

		constexpr bool isEmpty() const
		{
			return getWidth() <= 0 || getHeight() <= 0;
		}

		constexpr bool operator==(const Rect2D<T>& p) const
		{
			return p1 == p.p1 && p2 == p.p2;
		}

		constexpr bool operator!=(const Rect2D<T>& p) const
		{
			return p1 != p.p1 || p2 != p.p2;
		}

		template <typename V>
		constexpr Rect2D operator * (const V param) const
		{
			return Rect2D(p1 * param, p2 * param);
		}

		// HACK
		template <typename V>
		constexpr Rect2D mult(const V param) const
		{
			return Rect2D(p1 * param, p2 * param);
		}

		template <typename V>
		constexpr Rect2D operator / (const V param) const
		{
			return Rect2D(p1 / param, p2 / param);
		}

		constexpr Rect2D fitWithin(const Rect2D& container) const
		{
			Rect2D r = *this;
			if (r.p2.x > container.p2.x) {
				r -= Vector2D<T>(r.p2.x - container.p2.x, 0);
			}
			if (r.p2.y > container.p2.y) {
				r -= Vector2D<T>(0, r.p2.y - container.p2.y);
			}
			if (r.p1.x < container.p1.x) {
				r -= Vector2D<T>(r.p1.x - container.p1.x, 0);
			}
			if (r.p1.y < container.p1.y) {
				r -= Vector2D<T>(0, r.p1.y - container.p1.y);
			}
			return r;
		}

		String toString() const
		{
			return String("[") + p1.toString() + " " + p2.toString() + "]";
		}

		constexpr static std::optional<Rect2D> optionalIntersect(const std::optional<Rect2D>& a, const std::optional<Rect2D>& b)
		{
			if (a && b) {
				return a->intersection(b.value());
			} else if (a) {
				return a.value();
			} else if (b) {
				return b.value();
			} else {
				return {};
			}
		}

		static Rect2D getSpanningRect(const std::vector<Vector2D<T>>& points)
		{
			if (points.empty()) {
				return Rect2D(Vector2f(), Vector2f());
			}
			
			T x1 = std::numeric_limits<T>::max();
			T y1 = std::numeric_limits<T>::max();
			T x2 = std::numeric_limits<T>::lowest();
			T y2 = std::numeric_limits<T>::lowest();

			for (const auto& v: points) {
				x1 = std::min(x1, v.x);
				x2 = std::max(x2, v.x);
				y1 = std::min(y1, v.y);
				y2 = std::max(y2, v.y);
			}
			
			return Rect2D(Vector2f(x1, y1), Vector2f(x2, y2));
		}
	};

	template <typename T>
	std::ostream& operator<< (std::ostream& ostream, const Rect2D<T>& v)
	{
		ostream << "(" << v.getTopLeft().x << ", " << v.getTopLeft().y << ", " << v.getWidth() << ", " << v.getHeight() << ")";
		return ostream;
	}

	template <typename T, typename V>
	inline Rect2D<T> operator * (V f, Rect2D<T> v)
	{
		return v * f;
	}

	template <typename T, typename V>
	inline Rect2D<T> operator / (V f, Rect2D<T> v)
	{
		return v / f;
	}

	template <typename T, typename U>
	inline Rect2D<T> operator + (Vector2D<T, U> v, Rect2D<T> r)
	{
		return r + v;
	}

	typedef Rect2D<float> Rect4f;
	typedef Rect2D<int> Rect4i;
}
