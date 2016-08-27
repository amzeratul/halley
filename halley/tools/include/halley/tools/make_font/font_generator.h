#pragma once

#include "font_face.h"
#include <halley/maths/rect.h>
#include <boost/filesystem/path.hpp>

namespace Halley
{
	using Path = boost::filesystem::path;

	class FontGenerator
	{
		struct CharcodeEntry
		{
			int charcode = 0;
			Rect4i rect;

			CharcodeEntry() {}

			CharcodeEntry(int charcode, Rect4i rect)
				: charcode(charcode)
				, rect(rect)
			{}
		};

	public:
		void generateFont(Path fontFile, Path result, Vector2i size, float radius, int supersample, Range<int> range);

	private:
		void generateFontMapBinary(String imgName, FontFace& font, Vector<CharcodeEntry>& entries, Path outPath, float scale, float radius, Vector2i imageSize) const;
		void generateTextureMeta(Path destination);
	};
}
