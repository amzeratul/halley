#include "make_font_tool.h"
#include "font_face.h"
#include "../distance_field/distance_field_generator.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <filesystem>

using namespace Halley;

#ifdef _DEBUG
#define FAST_MODE
#endif

static Maybe<std::vector<BinPackResult>> tryPacking(FontFace& font, float fontSize, Vector2i packSize, float scale, float borderSuperSampled, Range<int> range)
{
	font.setSize(fontSize);
	std::cout << "Trying " << fontSize << " pt... ";

	std::vector<BinPackEntry> entries;
	for (int code : font.getCharCodes()) {
		if (range.contains(code)) {
			Vector2i glyphSize = font.getGlyphSize(code);
			int padding = int(2 * borderSuperSampled);
			Vector2i superSampleSize = glyphSize + Vector2i(padding, padding);
			Vector2i finalSize(Vector2f(superSampleSize) * scale + Vector2f(1, 1));

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
	int superSample = 4;
	Range<int> range(0, 256);
	float scale = 1.0f / superSample;
	float radius = String(args[3]).toFloat();
	float borderFinal = ceil(radius);
	float borderSuperSample = borderFinal * superSample;

	int minFont = 0;
	int maxFont = 200;
#ifdef FAST_MODE
	minFont = maxFont = 50;
#endif

	FontFace font(args[0]);
	auto result = binarySearch([&] (int fontSize) -> Maybe<std::vector<BinPackResult>> {
		return tryPacking(font, float(fontSize), size, scale, borderSuperSample, range);
	}, minFont, maxFont);

	auto dstImg = std::make_unique<Image>(size.x, size.y);
	dstImg->clear(0);

	std::vector<CharcodeEntry> codes;

	if (result) {
		auto& pack = result.get();
		std::cout << "Rendering " << pack.size() << " glyphs";

		for (auto& r: pack) {
			int charcode = int(reinterpret_cast<size_t>(r.data));
			Rect4i dstRect = r.rect;
			Rect4i srcRect = dstRect * superSample;

			auto tmpImg = std::make_unique<Image>(srcRect.getWidth(), srcRect.getHeight());
			tmpImg->clear(0);
			font.drawGlyph(*tmpImg, charcode, Vector2i(int(borderSuperSample), int(borderSuperSample)));

			auto finalGlyphImg = DistanceFieldGenerator::generate(*tmpImg, dstRect.getSize(), radius);
			dstImg->blitFrom(dstRect.getP1(), *finalGlyphImg);

			tmpImg.reset();
			finalGlyphImg.reset();

			std::cout << ".";

			codes.push_back(CharcodeEntry(charcode, dstRect));
		}
		std::cout << " Done." << std::endl;
	}

	using std::experimental::filesystem::path;
	path target = args[1];
	String fileName = target.filename().string();
	String dir = target.parent_path().string();
	String imgName = fileName + ".png";
	dstImg->savePNG(dir + "/" + imgName);
	generateYAML(imgName, font, codes, dir + "/" + fileName + ".yaml", scale, radius);

	return 0;
}

void MakeFontTool::generateYAML(String imgName, FontFace& font, std::vector<CharcodeEntry>& entries, String outPath, float scale, float radius) {
	std::sort(entries.begin(), entries.end(), [](const CharcodeEntry& a, const CharcodeEntry& b) { return a.charcode < b.charcode; });

	YAML::Emitter yaml;
	yaml << YAML::BeginMap;
	yaml << YAML::Key << "font";
	yaml << YAML::BeginMap;
	yaml << YAML::Key << "name" << YAML::Value << font.getName();
	yaml << YAML::Key << "image" << YAML::Value << imgName;
	yaml << YAML::Key << "sizePt" << YAML::Value << font.getSize();
	yaml << YAML::Key << "height" << YAML::Value << (font.getHeight() * scale);
	yaml << YAML::Key << "radius" << YAML::Value << radius;
	yaml << YAML::EndMap;
	yaml << YAML::Key << "glyphs";
	yaml << YAML::BeginSeq;

	for (auto& c : entries) {
		auto metrics = font.getMetrics(c.charcode, scale);
		String printable;
		printable.appendCharacter(c.charcode);

		yaml << YAML::BeginMap;
		yaml << YAML::Key << "code" << YAML::Value << c.charcode;
		yaml << YAML::Key << "character" << YAML::Value << YAML::DoubleQuoted << printable.c_str();
		yaml << YAML::Key << "x" << YAML::Value << c.rect.getX();
		yaml << YAML::Key << "y" << YAML::Value << c.rect.getY();
		yaml << YAML::Key << "w" << YAML::Value << c.rect.getWidth();
		yaml << YAML::Key << "h" << YAML::Value << c.rect.getHeight();
		yaml << YAML::Key << "horizontalBearingX" << YAML::Value << metrics.bearingHorizontal.x;
		yaml << YAML::Key << "horizontalBearingY" << YAML::Value << metrics.bearingHorizontal.y;
		yaml << YAML::Key << "verticalBearingX" << YAML::Value << metrics.bearingVertical.x;
		yaml << YAML::Key << "verticalBearingY" << YAML::Value << metrics.bearingVertical.y;
		yaml << YAML::Key << "advanceX" << YAML::Value << metrics.advance.x;
		yaml << YAML::Key << "advanceY" << YAML::Value << metrics.advance.y;
		yaml << YAML::EndMap;
	}

	yaml << YAML::EndSeq;
	yaml << YAML::EndMap;

	std::vector<int> codes;
	for (int code: font.getCharCodes()) {
		if (code < 256) {
			codes.push_back(code);
		}
	}
	for (auto& kern : font.getKerning(codes)) {
		std::cout << "Kerning: " << char(kern.left) << " " << char(kern.right) << ": " << kern.kerning << std::endl;
	}

	std::ofstream out(outPath, std::ios::out);
	out << yaml.c_str();
	out.close();
}