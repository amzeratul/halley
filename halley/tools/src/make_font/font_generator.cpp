#include "halley/tools/make_font/font_generator.h"
#include "halley/tools/distance_field/distance_field_generator.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <boost/filesystem.hpp>
#include <halley/data_structures/bin_pack.h>
#include <halley/file_formats/image.h>
#include <future>
#include "halley/file_formats/serializer.h"

using namespace Halley;

#ifdef _DEBUG
//#define FAST_MODE
#endif

static Maybe<Vector<BinPackResult>> tryPacking(FontFace& font, float fontSize, Vector2i packSize, float scale, float borderSuperSampled, Range<int> range)
{
	font.setSize(fontSize);
	std::cout << "Trying " << fontSize << " pt... ";

	Vector<BinPackEntry> entries;
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
	std::cout << (result ? "Fits." : "Does not fit.") << std::endl;
	return result;
}

static Maybe<Vector<BinPackResult>> binarySearch(std::function<Maybe<Vector<BinPackResult>>(int)> f, int minBound, int maxBound)
{
	int v0 = minBound;
	int v1 = maxBound;
	int lastGood = v0;
	Maybe<Vector<BinPackResult>> bestResult;

	while (v0 <= v1) {
		int v = (v0 + v1) / 2;
		Maybe<Vector<BinPackResult>> result = f(v);

		if (result) {
			// Midpoint is good, try increasing
			lastGood = v;
			bestResult = result;
			v0 = v + 1;
		}
		else {
			// Midpoint is too big, try decreasing
			v1 = v - 1;
		}
	}

	if (lastGood > 0) {
		std::cout << "Packing with " << lastGood << " pt." << std::endl;
	}
	return bestResult;
}

void FontGenerator::generateFont(Path fontFile, Path target, Vector2i size, float radius, int superSample, Range<int> range) {
	float scale = 1.0f / superSample;
	float borderFinal = ceil(radius);
	float borderSuperSample = borderFinal * superSample;

	int minFont = 0;
	int maxFont = 1000;
#ifdef FAST_MODE
	minFont = maxFont = 50;
#endif

	FontFace font(fontFile.string());
	auto result = binarySearch([&](int fontSize) -> Maybe<Vector<BinPackResult>> {
		return tryPacking(font, float(fontSize), size, scale, borderSuperSample, range);
	}, minFont, maxFont);

	auto dstImg = std::make_unique<Image>(size.x, size.y);
	dstImg->clear(0);

	Vector<CharcodeEntry> codes;
	Vector<std::future<void>> futures;
	std::mutex m;

	if (result) {
		auto& pack = result.get();
		std::cout << "Rendering " << pack.size() << " glyphs";

		for (auto& r : pack) {
			int charcode = int(reinterpret_cast<size_t>(r.data));
			Rect4i dstRect = r.rect;
			Rect4i srcRect = dstRect * superSample;
			codes.push_back(CharcodeEntry(charcode, dstRect));

			futures.push_back(std::async(std::launch::async, [=, &m, &font, &dstImg] {
				std::cout << "+";

				auto tmpImg = std::make_unique<Image>(srcRect.getWidth(), srcRect.getHeight());
				tmpImg->clear(0);
				{
					std::lock_guard<std::mutex> g(m);
					font.drawGlyph(*tmpImg, charcode, Vector2i(int(borderSuperSample), int(borderSuperSample)));
				}

				auto finalGlyphImg = DistanceFieldGenerator::generate(*tmpImg, dstRect.getSize(), radius);
				dstImg->blitFrom(dstRect.getTopLeft(), *finalGlyphImg);

				tmpImg.reset();
				finalGlyphImg.reset();

				std::cout << "-";
			}));
		}
	}
	std::sort(codes.begin(), codes.end(), [](const CharcodeEntry& a, const CharcodeEntry& b) { return a.charcode < b.charcode; });

	for (auto& f : futures) {
		f.get();
	}
	std::cout << " Done generating." << std::endl;

	Path fileName = target.filename();
	Path dir = target.parent_path();
	Path imgName = change_extension(fileName, ".png");
	Path pngPath = dir / imgName;
	Path yamlPath = dir / change_extension(fileName, ".yaml");
	Path binPath = dir / change_extension(fileName, ".font");
	Path metaPath = change_extension(pngPath, ".png.meta");
	std::cout << "Saving " << pngPath << ", " << yamlPath << ", and " << metaPath << std::endl;

	dstImg->savePNG(pngPath.string());

	generateFontMapBinary(imgName.string(), font, codes, binPath, scale, radius, size);
	generateTextureMeta(metaPath.string());
}

void FontGenerator::generateFontMapBinary(String imgName, FontFace& font, Vector<CharcodeEntry>& entries, Path outPath, float scale, float radius, Vector2i imageSize) const
{
	auto serialize = [&] (Serializer& s) {
		String name = font.getName();
		String imageName = imgName;
		float ascender = (font.getAscender() * scale);
		float height = (font.getHeight() * scale);
		float sizePt = (font.getSize() * scale);
		float smoothRadius = radius;
		s << name;
		s << imageName;
		s << ascender;
		s << height;
		s << sizePt;
		s << smoothRadius;

		unsigned int numEntries = static_cast<unsigned int>(entries.size());
		s << numEntries;
		for (unsigned int i = 0; i < numEntries; i++) {
			auto& c = entries[i];
			auto metrics = font.getMetrics(c.charcode, scale);

			int charcode = c.charcode;
			Rect4f area = Rect4f(c.rect) / Vector2f(imageSize);
			Vector2f size = Vector2f(c.rect.getSize());
			Vector2f horizontalBearing = metrics.bearingHorizontal;
			Vector2f verticalBearing = metrics.bearingVertical;
			Vector2f advance = metrics.advance;
			s << charcode;
			s << area;
			s << size;
			s << horizontalBearing;
			s << verticalBearing;
			s << advance;
		}
	};
	
	auto bytes = Serializer::toBytes(serialize);
	std::ofstream out(outPath.string(), std::ios::out | std::ios::binary);
	out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	out.close();
}

void FontGenerator::generateTextureMeta(Path outPath)
{
	YAML::Emitter yaml;
	yaml << YAML::BeginMap;
	yaml << YAML::Key << "filtering" << YAML::Value << true;
	yaml << YAML::Key << "mipmap" << YAML::Value << false;
	yaml << YAML::Key << "premultiply" << YAML::Value << false;
	yaml << YAML::Key << "format" << YAML::Value << "RGBA";
	yaml << YAML::EndMap;

	std::ofstream out(outPath.string(), std::ios::out);
	out << "---\n";
	out << yaml.c_str();
	out << "\n...\n";
	out.close();
}
