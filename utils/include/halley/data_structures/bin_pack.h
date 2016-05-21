#pragma once
#include "../maths/vector2d.h"
#include "../maths/rect.h"
#include "maybe.h"

namespace Halley
{
	class BinPackEntry
	{
	public:
		BinPackEntry(Vector2i size, void* data = nullptr)
			: size(size)
			, data(data)
		{}

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
		static Maybe<std::vector<BinPackResult>> pack(std::vector<BinPackEntry> entries, Vector2i binSize);
	};
}
