#pragma once

#include "font_face.h"
#include <halley/maths/rect.h>
#include "halley/file/path.h"
#include "halley/resources/metadata.h"
#include "halley/data_structures/maybe.h"

namespace Halley
{
	class Font;

	struct FontGeneratorResult
	{
		bool success = false;
		std::unique_ptr<Image> image;
		std::unique_ptr<Metadata> imageMeta;
		std::unique_ptr<Font> font;

		FontGeneratorResult();
		FontGeneratorResult(FontGeneratorResult&& other) noexcept;
		~FontGeneratorResult();
		std::vector<Path> write(Path dst, bool verbose = false) const;
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
		struct FontSizeInfo
		{
			std::optional<Vector2i> imageSize;
			std::optional<float> fontSize;
			float replacementScale = 1.0f;
		};

		explicit FontGenerator(bool verbose = false, std::function<bool(float, String)> progressReporter = ignoreReport);
		FontGeneratorResult generateFont(const Metadata& meta, gsl::span<const gsl::byte> fontFile, FontSizeInfo sizeInfo, float radius, int supersample, std::vector<int> characters);

	private:
		std::unique_ptr<Font> generateFontMapBinary(const Metadata& meta, FontFace& font, Vector<CharcodeEntry>& entries, float scale, float renderScale, float radius, Vector2i imageSize) const;
		static std::unique_ptr<Metadata> generateTextureMeta();

		bool verbose;
		std::function<bool(float, String)> progressReporter;
	};
}
