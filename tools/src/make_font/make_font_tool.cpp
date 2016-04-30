#include "make_font_tool.h"
#include "font_face.h"
#include "../distance_field/distance_field_generator.h"

using namespace Halley;

#ifdef _DEBUG
//#define FAST_MODE
#endif

static Maybe<std::vector<BinPackResult>> tryPacking(FontFace& font, float fontSize, Vector2i packSize, float scale, float border, Range<int> range)
{
	font.setSize(fontSize);
	std::cout << "Trying " << fontSize << " pt... ";

	std::vector<BinPackEntry> entries;
	for (int code : font.getCharCodes()) {
		if (range.contains(code)) {
			Vector2i glyphSize = font.getGlyphSize(code);
			Vector2i finalSize((Vector2f(glyphSize) + Vector2f(2 * border, 2 * border)) * scale + Vector2f(1, 1));
			size_t payload = size_t(code);
			entries.push_back(BinPackEntry(finalSize, reinterpret_cast<void*>(payload)));

#ifdef FAST_MODE
			if (entries.size() > 50) {
				break;
			}
#endif
		}
	}

	auto result = BinPack::pack(entries, packSize);
	std::cout << (result? "Fits." : "Does not fit.") << std::endl;
	return result;
}

static Maybe<std::vector<BinPackResult>> binarySearch(std::function<Maybe<std::vector<BinPackResult>>(int)> f, int minBound, int maxBound)
{
	int v0 = minBound;
	int v1 = maxBound;
	int lastGood = v0;
	Maybe<std::vector<BinPackResult>> bestResult;
	
	while (v0 <= v1) {
		int v = (v0 + v1) / 2;
		Maybe<std::vector<BinPackResult>> result = f(v);

		if (result) {
			// Midpoint is good, try increasing
			lastGood = v;
			bestResult = result;
			v0 = v + 1;
		} else {
			// Midpoint is too big, try decreasing
			v1 = v - 1;
		}
	}

	if (lastGood > 0) {
		std::cout << "Packing with " << lastGood << " pt." << std::endl;
	}
	return bestResult;
}

int MakeFontTool::run(std::vector<std::string> args)
{
	if (args.size() != 4) {
		std::cout << "Usage: halley-cmd makeFont srcFont resultName WxH radius" << std::endl;
		return 1;
	}

	auto res = String(args[2]).split('x');
	Vector2i size(res[0].toInteger(), res[1].toInteger());
	int downsample = 4;
	Range<int> range(32, 255);
	float scale = 1.0f / downsample;
	float radius = 3;
	float border = radius + 1 + downsample;

	int minFont = 0;
	int maxFont = 200;
#ifdef FAST_MODE
	minFont = maxFont = 50;
#endif

	FontFace font(args[0]);
	auto result = binarySearch([&] (int fontSize) -> Maybe<std::vector<BinPackResult>> {
		return tryPacking(font, float(fontSize), size, scale, border, range);
	}, minFont, maxFont);

	auto dstImg = std::make_unique<Image>(size.x, size.y);
	dstImg->clear(0);

	if (result) {
		auto& pack = result.get();
		std::cout << "Rendering " << pack.size() << " glyphs." << std::endl;

		for (auto& r: pack) {
			int charcode = int(reinterpret_cast<size_t>(r.data));
			Rect4i dstRect = r.rect;
			Rect4i srcRect = dstRect * downsample;

			auto tmpImg = std::make_unique<Image>(srcRect.getWidth(), srcRect.getHeight());
			tmpImg->clear(0);
			font.drawGlyph(*tmpImg, charcode, Vector2i(int(border), int(border)));

			auto finalGlyphImg = DistanceFieldGenerator::generate(*tmpImg, dstRect.getSize(), radius);
			dstImg->blitFrom(dstRect.getP1(), *finalGlyphImg);

			tmpImg.reset();
			finalGlyphImg.reset();
		}
	}

	dstImg->savePNG(args[1] + ".png");

	return 0;
}
