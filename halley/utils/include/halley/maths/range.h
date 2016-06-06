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

namespace Halley {

	template <typename T>
	class Range {
	public:
		T s;
		T e;

		Range(T start, T end) {
			if (start < end) {
				s = start;
				e = end;
			} else {
				s = end;
				e = start;
			}
		}

		bool contains(T elem) const
		{
			return elem >= s && elem < e;
		}

		inline T getLength() { return e - s; }

		inline bool overlaps(const Range &p) const
		{
			return (s < p.e) && (p.s < e);
		}

		inline Range getOverlap(const Range &p) const
		{
			T start = std::max(s, p.s);
			T end = std::min(e, p.e);
			if (start < end) {
				return Range(start, end);
			} else {
				T avg = (start+end)/2;
				return Range(avg, avg);
			}
		}
		
		inline Range getSweepOverlap(T len) const
		{
			return Range(s - len, e);
		}

		T getTimeToOverlap(const Range& p, T factor=1) const
		{
			T sx = p.s;
			T ex = p.e;
			T divisor = ex - sx;
			if (divisor == 0) return 0;

			T t1 = (s-sx)*factor/divisor;
			T t2 = (e-sx)*factor/divisor;
			return min(abs(t1), abs(t2));
		}
	};
}
