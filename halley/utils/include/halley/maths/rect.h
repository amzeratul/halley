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

namespace Halley {
	//////////////////////////////
	// Vector2D class declaration
	template <typename T=float>
	class Rect2D {
		Vector2D<T> p1, p2;

	public:
		Rect2D() = default;
		Rect2D(const Rect2D<T>& r) = default;
		Rect2D(Rect2D<T>&& r) = default;

		Rect2D<T>& operator=(const Rect2D<T>& o) = default;
		Rect2D<T>& operator=(Rect2D<T>&& o) = default;

		template<typename U> explicit Rect2D(Rect2D<U> r)
		{
			set(T(r.getX()), T(r.getY()), T(r.getWidth()), T(r.getHeight()));
		}

		Rect2D(Vector2D<T> point1, Vector2D<T> point2)
		{
			set(point1, point2);
		}

		Rect2D(Vector2D<T> origin, T width, T height)
		{
			set(origin, Vector2D<T>(origin.x+width, origin.y+height));
		}

		Rect2D(T x, T y, T width, T height)
		{
			set(x, y, width, height);
		}

		void set(T x, T y, T width, T height)
		{
			set(Vector2D<T>(x, y), Vector2D<T>(x+width, y+height));
		}

		void set(Vector2D<T> point1, Vector2D<T> point2)
		{
			p1.x = std::min(point1.x, point2.x);
			p1.y = std::min(point1.y, point2.y);
			p2.x = std::max(point1.x, point2.x);
			p2.y = std::max(point1.y, point2.y);
		}

		void setPos(Vector2D<T> pos)
		{
			auto sz = p2 - p1;
			p1 = pos;
			p2 = pos + sz;
		}

		void setX(T x)
		{
			float dx = x - p1.x;
			p1.x += dx;
			p2.x += dx;
		}

		void setY(T y)
		{
			float dy = y - p1.y;
			p1.y += dy;
			p2.y += dy;
		}

		void setWidth(T w)
		{
			p2.x = p1.x + w;
		}

		void setHeight(T h)
		{
			p2.y = p1.y + h;
		}

		void setSize(Vector2D<T> size)
		{
			p2 = p1 + size;
		}

		Rect2D<T> shrink(T amount)
		{
			auto offset = Vector2D<T>(amount, amount);
			return Rect2D(p1 + offset, p2 - offset);
		}

		Rect2D<T> grow(T amount)
		{
			auto offset = Vector2D<T>(amount, amount);
			return Rect2D(p1 - offset, p2 + offset);
		}

		Vector2D<T> getTopLeft() const { return p1; }
		Vector2D<T> getBottomRight() const { return p2; }
		Vector2D<T> getBottomLeft() const { return Vector2D<T>(p1.x, p2.y); }
		Vector2D<T> getTopRight() const { return Vector2D<T>(p2.x, p1.y); }
		Vector2D<T> getSize() const { return p2 - p1; }

		T getWidth() const { return p2.x - p1.x; }
		T getHeight() const { return p2.y - p1.y; }
		T getX() const { return p1.x; }
		T getY() const { return p1.y; }

		T getLeft() const { return p1.x; }
		T getRight() const { return p2.x; }
		T getTop() const { return p1.y; }
		T getBottom() const { return p2.y; }

		bool isInside(Vector2D<T> p) const
		{
			return (p.x >= p1.x && p.x <= p2.x && p.y >= p1.y && p.y <= p2.y);
		}

		Vector2D<T> wrapInside(Vector2D<T> p) const
		{
			Vector2D<T> size = getSize();
			return ((((p-p1) % size) + size) % size) + p1;
		}

		Rect2D<T> intersection(Rect2D<T> p) const
		{
			Range<T> x0(p1.x, p2.x);
			Range<T> x1(p.p1.x, p.p2.x);
			Range<T> x = x0.getOverlap(x1);
			Range<T> y0(p1.y, p2.y);
			Range<T> y1(p.p1.y, p.p2.y);
			Range<T> y = y0.getOverlap(y1);
			return Rect2D<T>(Vector2D<T>(x.s, y.s), Vector2D<T>(x.e, y.e));
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

		Rect2D<T> operator+(Vector2D<T> v) const
		{
			return Rect2D<T>(p1 + v, p2 + v);
		}

		Rect2D<T> operator-(Vector2D<T> v) const
		{
			return Rect2D<T>(p1 - v, p2 - v);
		}

		Rect2D<T>& operator+=(Vector2D<T> v)
		{
			p1 += v;
			p2 += v;
			return *this;
		}

		Rect2D<T>& operator-=(Vector2D<T> v)
		{
			p1 -= v;
			p2 -= v;
			return *this;
		}

		bool overlaps(Rect2D<T> other) const
		{
			return !(p2.x <= other.p1.x || other.p2.x <= p1.x || p2.y <= other.p1.y || other.p2.y <= p1.y);
		}

		Vector2D<T> getCenter() const
		{
			return (p1+p2)/2;
		}

		bool isEmpty() const
		{
			return getWidth() <= 0 || getHeight() <= 0;
		}

		bool operator==(const Rect2D<T>& p) const
		{
			return p1 == p.p1 && p2 == p.p2;
		}

		bool operator!=(const Rect2D<T>& p) const
		{
			return p1 != p.p1 || p2 != p.p2;
		}

		template <typename V>
		Rect2D operator * (const V param) const
		{
			return Rect2D(p1 * param, p2 * param);
		}

		template <typename V>
		Rect2D operator / (const V param) const
		{
			return Rect2D(p1 / param, p2 / param);
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