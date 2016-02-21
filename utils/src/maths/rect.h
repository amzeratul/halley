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

#include "vector2d.h"
#include "range.h"

namespace Halley {
	//////////////////////////////
	// Vector2D class declaration
	template <typename T=float>
	class Rect2D {
		Vector2D<T> p1, p2;

	public:
		Rect2D() {}

		template<typename U> explicit Rect2D(Rect2D<U> r)
		{
			set((T)r.getX(), (T)r.getY(), (T)r.getWidth(), (T)r.getHeight());
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
		void setWidth(T w) { p2.x = p1.x + w; }
		void setHeight(T h) { p2.y = p1.y + h; }

		Vector2D<T> getP1() const { return p1; }
		Vector2D<T> getP2() const { return p2; }
		Vector2D<T>& getP1() { return p1; }
		Vector2D<T>& getP2() { return p2; }
		Vector2D<T> getSize() const { return p2-p1; }
		T getWidth() const { return p2.x - p1.x; }
		T getHeight() const { return p2.y - p1.y; }
		T getX() const { return p1.x; }
		T getY() const { return p1.y; }

		bool isInside(Vector2D<T> p) const
		{
			return (p.x >= p1.x && p.x <= p2.x && p.y >= p1.y && p.y <= p2.y);
		}

		Vector2D<T> wrapInside(Vector2D<T> p) const
		{
			Vector2D<T> size = getSize();
			return ((((p-p1) % size) + size) % size) + p1;
		}

		Rect2D<T> intersect(Rect2D<T> p) const
		{
			Range<T> x0(p1.x, p2.x);
			Range<T> x1(p.p1.x, p.p2.x);
			Range<T> x = x0.getOverlap(x1);
			Range<T> y0(p1.y, p2.y);
			Range<T> y1(p.p1.y, p.p2.y);
			Range<T> y = y0.getOverlap(y1);
			return Rect2D<T>(Vector2D<T>(x.s, y.s), Vector2D<T>(x.e, y.e));
		}

		bool intersects(Rect2D<T> other) const
		{
			return !(p2.x <= other.p1.x || other.p2.x <= p1.x || p2.y <= other.p1.y || other.p2.y <= p1.y);
		}

		Vector2D<T> getCenter() const
		{
			return (p1+p2)/2;
		}

		bool isEmpty() const { return getWidth() <= 0 || getHeight() <= 0; }

		bool operator==(const Rect2D<T>& p) const { return p1 == p.p1 && p2 == p.p2; }
		bool operator!=(const Rect2D<T>& p) const { return p1 != p.p1 || p2 != p.p2; }
	};

	typedef Rect2D<float> Rect4f;
	typedef Rect2D<int> Rect4i;
}