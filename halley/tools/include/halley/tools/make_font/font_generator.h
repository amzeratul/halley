#pragma once

#include "font_face.h"
#include <halley/maths/rect.h>
#include "halley/file/filesystem.h"
#include "halley/resources/metadata.h"

namespace Halley
{
	struct FontGeneratorResult
	{
		bool success = false;
		String assetName;
		std::unique_ptr<Image> image;
		std::unique_ptr<Metadata> imageMeta;
		Bytes fontData;

		FontGeneratorResult();
		FontGeneratorResult(FontGeneratorResult&& other);
		~FontGeneratorResult();
		std::vector<filesystem::path> write(Path dst, bool verbose = false) const;
	};

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

		static bool ignoreReport(float, String) { return true; }

	public:
		explicit FontGenerator(bool verbose = false, std::function<bool(float, String)> progressReporter = ignoreReport);
		FontGeneratorResult generateFont(String assetName, Path fontFile, Vector2i size, float radius, int supersample, Range<int> range);

	private:
		Bytes generateFontMapBinary(String imgName, FontFace& font, Vector<CharcodeEntry>& entries, float scale, float radius, Vector2i imageSize) const;
		static std::unique_ptr<Metadata> generateTextureMeta();

		bool verbose;
		std::function<bool(float, String)> progressReporter;
	};
}
