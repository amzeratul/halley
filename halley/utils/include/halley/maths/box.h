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

#include <halley/utils/utils.h>
#include "vector3d.h"
#include "range.h"

namespace Halley {
	template <typename T>
	class Box {
	public:
		Vector3D<T> start;
		Vector3D<T> size;

		Box(Vector3D<T> _start, Vector3D<T> _size)
			: start(_start), size(_size)
		{}

		inline bool overlaps(const Box& p) const
		{
			Vector3D<T> s1 = start;
			Vector3D<T> e1 = s1+size;

			Vector3D<T> s2 = p.start;
			Vector3D<T> e2 = s2+p.size;

			return (rangeIntersection(s1.z, e1.z, s2.z, e2.z)
				&& rangeIntersection(s1.y, e1.y, s2.y, e2.y)
				&& rangeIntersection(s1.x, e1.x, s2.x, e2.x));
		}

		inline Vector3i getSeparatingAxes(const Box& p) const
		{
			Vector3D<T> s1 = start;
			Vector3D<T> e1 = s1+size;

			Vector3D<T> s2 = p.start;
			Vector3D<T> e2 = s2+p.size;

			return Vector3i(
				rangeIntersection(s1.x, e1.x, s2.x, e2.x) ? 0 : 1,
				rangeIntersection(s1.y, e1.y, s2.y, e2.y) ? 0 : 1, 
				rangeIntersection(s1.z, e1.z, s2.z, e2.z) ? 0 : 1);
		}

		inline Vector3i getOverlapFactor(const Box& s, const Box& e) const
		{
			Range<T> rx(start.x-s.size.x, start.x+size.x);
			Range<T> ry(start.y-s.size.y, start.y+size.y);
			Range<T> rz(start.z-s.size.z, start.z+size.z);
			Range<T> mx(s.start.x, e.start.x);
			Range<T> my(s.start.y, e.start.y);
			Range<T> mz(s.start.z, e.start.z);

			return Vector3i(
					rx.getTimeToOverlap(mx, 255),
					ry.getTimeToOverlap(my, 255),
					rz.getTimeToOverlap(mz, 255)
				);
		}

		inline Vector3D<T> getOverlapRange(const Box& p) const
		{
			Vector3D<T> s1 = start;
			Vector3D<T> e1 = s1+size;

			Vector3D<T> s2 = p.start;
			Vector3D<T> e2 = s2+p.size;

			Vector3D<T> a = e1-s2;
			Vector3D<T> b = s1-e2;

			return Vector3D<T>((a.x * b.x < 0) ? minAbs(a.x, b.x) : 0,
				(a.y * b.y < 0) ? minAbs(a.y, b.y) : 0,
				(a.z * b.z < 0) ? minAbs(a.z, b.z) : 0);
		}

		inline Vector3D<T> getOverlapRange(const Box& p, Vector3D<T> dirs) const
		{
			Vector3D<T> s1 = start;
			Vector3D<T> e1 = s1+size;

			Vector3D<T> s2 = p.start;
			Vector3D<T> e2 = s2+p.size;

			Vector3D<T> a = e1-s2;
			Vector3D<T> b = s1-e2;

			return Vector3D<T>((a.x * b.x < 0) ? (dirs.x ? b.x : a.x) : 0,
				(a.y * b.y < 0) ? (dirs.y ? b.y : a.y) : 0,
				(a.z * b.z < 0) ? (dirs.z ? b.z : a.z) : 0);
		}
	};

	typedef Box<float> Box3f;
	typedef Box<int> Box3i;
}