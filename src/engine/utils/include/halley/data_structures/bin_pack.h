#pragma once
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"
#include <boost/optional.hpp>

namespace Halley
{
	class BinPackEntry
	{
	public:
		BinPackEntry(Vector2i size, void* data = nullptr)
			: size(size)
			, data(data)
		{}

		bool operator<(const BinPackEntry& other) const
		{
			int aMaj = std::max(size.x, size.y);
			int aMin = std::min(size.x, size.y);
			int bMaj = std::max(other.size.x, other.size.y);
			int bMin = std::min(other.size.x, other.size.y);
			if (aMaj != bMaj) {
				return aMaj > bMaj;
			}
			return aMin > bMin;
		}

		Vector2i size;
		void* data;
	};

	class BinPackResult
	{
	public:
		BinPackResult(Rect4i rect, bool rotated, void* data)
			: rect(rect)
			, rotated(rotated)
			, data(data)
		{}

		Rect4i rect;
		bool rotated;
		void* data;
	};

	class BinPack
	{
	public:
		static boost::optional<Vector<BinPackResult>> pack(const std::vector<BinPackEntry>& entries, Vector2i binSize);
		static boost::optional<Vector<BinPackResult>> fastPack(const std::vector<BinPackEntry>& entries, Vector2i binSize);
	};
}
