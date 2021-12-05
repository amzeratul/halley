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

#include <algorithm>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace Halley {

	template <typename T>
	class Range {
	public:
		T start;
		T end;

		constexpr Range()
			: start(0)
			, end(0)
		{}

		constexpr Range(T _start, T _end) {
			if (_start < _end) {
				start = _start;
				end = _end;
			} else {
				start = _end;
				end = _start;
			}
		}

		constexpr bool contains(T elem) const
		{
			return elem >= start && elem < end;
		}

		constexpr bool contains(const Range& p) const
		{
			return (start <= p.start) && (end >= p.end);
		}

		constexpr T getLength() const { return end - start; }

		constexpr bool overlaps(const Range &p) const
		{
			return (start < p.end) && (p.start < end);
		}

		constexpr Range getOverlap(const Range &p) const
		{
			T s = std::max(start, p.start);
			T e = std::min(end, p.end);
			if (s < e) {
				return Range(s, e);
			} else {
				T avg = (s + e) / 2;
				return Range(avg, avg);
			}
		}
		
		constexpr Range getSweepOverlap(T len) const
		{
			return Range(start - len, end);
		}

		constexpr T getTimeToOverlap(const Range& p, T factor=1) const
		{
			T sx = p.start;
			T ex = p.end;
			T divisor = ex - sx;
			if (divisor == 0) return 0;

			T t1 = (start-sx)*factor/divisor;
			T t2 = (end-sx)*factor/divisor;
			return std::min(std::abs(t1), std::abs(t2));
		}

		constexpr bool operator==(const Range& other) const
		{
			return start == other.start && end == other.end;
		}

		constexpr bool operator!=(const Range& other) const
		{
			return start != other.start || end != other.end;
		}

		Range getUnion(const Range& other) const
		{
			return Range(std::min(start, other.start), std::max(end, other.end));
		}
	};
}
