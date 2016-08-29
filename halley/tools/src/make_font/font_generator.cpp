#include <fstream>
#include <future>
#include <cstdint>
#include <atomic>
#include <boost/filesystem.hpp>
#include <yaml-cpp/yaml.h>

#include "halley/tools/make_font/font_generator.h"
#include "halley/tools/distance_field/distance_field_generator.h"
#include <halley/data_structures/bin_pack.h>
#include <halley/file_formats/image.h>
#include "halley/file/byte_serializer.h"
#include "halley/file/filesystem.h"

using namespace Halley;

#ifdef _DEBUG
//#define FAST_MODE
#endif

static Maybe<Vector<BinPackResult>> tryPacking(FontFace& font, float fontSize, Vector2i packSize, float scale, float borderSuperSampled, Range<int> range)
{
	font.setSize(fontSize);

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

	return BinPack::pack(entries, packSize);
}

static Maybe<Vector<BinPackResult>> binarySearch(std::function<Maybe<Vector<BinPackResult>>(int)> f, int minBound, int maxBound, int &best)
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

	best = lastGood;
	return bestResult;
}

FontGenerator::FontGenerator(bool verbose, std::function<bool(float, String)> progressReporter)
	: verbose(verbose)
	, progressReporter(progressReporter)
{
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

	if (!progressReporter(0, "Packing")) {
		return;
	}

	FontFace font(fontFile.string());
	if (verbose) {
		std::cout << "Finding best pack size...\n";
	}
	int fontSize;
	auto result = binarySearch([&](int fontSize) -> Maybe<Vector<BinPackResult>> {
		return tryPacking(font, float(fontSize), size, scale, borderSuperSample, range);
	}, minFont, maxFont, fontSize);
	font.setSize(float(fontSize));
	
	if (!progressReporter(0.1f, "Encoding")) {
		return;
	}

	auto dstImg = std::make_unique<Image>(size.x, size.y);
	dstImg->clear(0);

	Vector<CharcodeEntry> codes;
	Vector<std::future<void>> futures;
	std::mutex m;
	std::atomic<int> nDone(0);
	std::atomic<bool> keepGoing(true);

	if (result) {
		auto& pack = result.get();
		if (verbose) {
			std::cout << "Rendering " << pack.size() << " glyphs";
		}

		for (auto& r : pack) {
			int charcode = int(reinterpret_cast<size_t>(r.data));
			Rect4i dstRect = r.rect;
			Rect4i srcRect = dstRect * superSample;
			codes.push_back(CharcodeEntry(charcode, dstRect));

			futures.push_back(std::async(std::launch::async, [=, &m, &font, &dstImg, &nDone, &keepGoing] {
				if (!keepGoing) {
					return;
				}

				if (verbose) {
					std::cout << "+";
				}

				auto tmpImg = std::make_unique<Image>(srcRect.getWidth(), srcRect.getHeight());
				tmpImg->clear(0);
				{
					std::lock_guard<std::mutex> g(m);
					font.drawGlyph(*tmpImg, charcode, Vector2i(int(borderSuperSample), int(borderSuperSample)));
				}

				if (!keepGoing) {
					return;
				}
				auto finalGlyphImg = DistanceFieldGenerator::generate(*tmpImg, dstRect.getSize(), radius);
				dstImg->blitFrom(dstRect.getTopLeft(), *finalGlyphImg);

				tmpImg.reset();
				finalGlyphImg.reset();

				if (verbose) {
					std::cout << "-";
				}
				float progress = lerp(0.1f, 0.95f, float(++nDone) / float(pack.size()));
				
				if (!progressReporter(progress, "Generating")) {
					keepGoing = false;
				}
			}));
		}
	} else {
		throw Exception("Unable to generate font.");
	}
	std::sort(codes.begin(), codes.end(), [](const CharcodeEntry& a, const CharcodeEntry& b) { return a.charcode < b.charcode; });

	for (auto& f : futures) {
		f.get();
	}
	if (!keepGoing) {
		return;
	}

	if (verbose) {
		std::cout << " Done generating." << std::endl;
	}
	if (!progressReporter(0.95f, "Generating files")) {
		return;
	}

	Path fileName = target.filename();
	Path dir = target.parent_path();
	Path imgName = change_extension(fileName, ".png");
	Path pngPath = dir / imgName;
	Path binPath = dir / change_extension(fileName, ".font");
	Path metaPath = change_extension(pngPath, ".png.meta");
	if (verbose) {
		std::cout << "Saving " << pngPath << ", " << binPath << ", and " << metaPath << std::endl;
	}

	dstImg->savePNG(pngPath.string());

	generateFontMapBinary(imgName.string(), font, codes, binPath, scale, radius, size);
	generateTextureMeta(metaPath.string());
	progressReporter(1.0f, "Done");
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

			int32_t charcode = c.charcode;
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
	
	FileSystem::writeFile(outPath, Serializer::toBytes(serialize));
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
