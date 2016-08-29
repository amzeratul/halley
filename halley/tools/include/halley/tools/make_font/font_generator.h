#pragma once

#include "font_face.h"
#include <halley/maths/rect.h>
#include "halley/file/filesystem.h"

namespace Halley
{
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

		static void ignoreReport(float, String) {}

	public:
		explicit FontGenerator(bool verbose = false, std::function<void(float, String)> progressReporter = ignoreReport);
		void generateFont(Path fontFile, Path result, Vector2i size, float radius, int supersample, Range<int> range);

	private:
		void generateFontMapBinary(String imgName, FontFace& font, Vector<CharcodeEntry>& entries, Path outPath, float scale, float radius, Vector2i imageSize) const;
		void generateTextureMeta(Path destination);

		bool verbose;
		std::function<void(float, String)> progressReporter;
	};
}
