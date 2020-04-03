#pragma once
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"
#include <halley/data_structures/maybe.h>

namespace Halley
{
	class BinPackEntry
	{
	public:
		BinPackEntry(Vector2i size, void* data = nullptr, bool canRotate = false)
			: size(size)
			, data(data)
			, canRotate(canRotate)
		{}

		bool operator<(const BinPackEntry& other) const
		{
			if (canRotate) {
				int aMaj = std::max(size.x, size.y);
				int aMin = std::min(size.x, size.y);
				int bMaj = std::max(other.size.x, other.size.y);
				int bMin = std::min(other.size.x, other.size.y);
				if (aMaj != bMaj) {
					return aMaj < bMaj;
				}
				return aMin < bMin;
			} else {
				if (size.y != other.size.y) {
					return size.y < other.size.y;
				}
				return size.x < other.size.x;
			}
		}

		Vector2i size;
		void* data;
		bool canRotate;
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
		static std::optional<Vector<BinPackResult>> pack(const std::vector<BinPackEntry>& entries, Vector2i binSize);
		static std::optional<Vector<BinPackResult>> fastPack(const std::vector<BinPackEntry>& entries, Vector2i binSize);
	};
}
