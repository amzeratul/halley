#include <fstream>
#include <future>
#include <cstdint>
#include <atomic>

#include "halley/tools/make_font/font_generator.h"
#include "halley/tools/distance_field/distance_field_generator.h"
#include <halley/data_structures/bin_pack.h>
#include <halley/file_formats/image.h>
#include "halley/bytes/byte_serializer.h"
#include "halley/file/path.h"
#include "halley/concurrency/concurrent.h"
#include "halley/tools/file/filesystem.h"
#include "halley/graphics/text/font.h"
#include "halley/support/logger.h"

using namespace Halley;

static std::optional<Vector<BinPackResult>> tryPacking(FontFace& font, float fontSize, Vector2i packSize, float scale, float borderSuperSampled, const Vector<int>& characters)
{
	font.setSize(fontSize);

	Vector<BinPackEntry> entries;
	for (int code : font.getCharCodes()) {
		if (std::binary_search(characters.begin(), characters.end(), code)) {
			Vector2i glyphSize = font.getGlyphSize(code);
			int padding = int(2 * borderSuperSampled) + 1;
			Vector2i superSampleSize = glyphSize + Vector2i(padding, padding);
			Vector2i finalSize(Vector2f(superSampleSize) * scale + Vector2f(1, 1));

			size_t payload = size_t(code);
			entries.push_back(BinPackEntry(finalSize, reinterpret_cast<void*>(payload)));
		}
	}

	constexpr bool fastPack = true;
	if (fastPack) {
		return BinPack::fastPack(entries, packSize);
	} else {
		return BinPack::pack(entries, packSize);
	}
}

static std::optional<Vector<BinPackResult>> binarySearch(std::function<std::optional<Vector<BinPackResult>>(int)> f, int minBound, int maxBound, int &best)
{
	int v0 = minBound;
	int v1 = maxBound;
	int lastGood = v0;
	std::optional<Vector<BinPackResult>> bestResult;

	while (v0 <= v1) {
		int v = (v0 + v1) / 2;
		std::optional<Vector<BinPackResult>> result = f(v);

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

FontGeneratorResult FontGenerator::generateFont(const Metadata& meta, gsl::span<const gsl::byte> fontFile, FontSizeInfo sizeInfo, float radius, int superSample, Vector<int> characters) {
	std::sort(characters.begin(), characters.end());

	const float scale = 1.0f / superSample;
	const float borderFinal = ceil(radius);
	const float borderSuperSample = borderFinal * superSample;

	if (!progressReporter(0, "Packing")) {
		return FontGeneratorResult();
	}

	FontFace font(fontFile);

	int fontSize = 0;
	Vector2i imageSize;
	std::optional<Vector<BinPackResult>> result;

	if (sizeInfo.fontSize) {
		fontSize = int(sizeInfo.fontSize.value());

		constexpr int minSize = 16;
		constexpr int maxSize = 4096;
		for (int i = 0; ; ++i) {
			auto curSize = Vector2i(minSize << ((i + 1) / 2), minSize << (i / 2));
			if (curSize.x > maxSize || curSize.y > maxSize) {
				break;
			}
			result = tryPacking(font, float(fontSize), curSize, scale, borderSuperSample, characters);
			if (result) {
				imageSize = curSize;
				break;
			}
		}
	} else if (sizeInfo.imageSize) {
		imageSize = sizeInfo.imageSize.value();
		
		constexpr int minFont = 0;
		constexpr int maxFont = 1000;
		result = binarySearch([&](int curFontSize) -> std::optional<Vector<BinPackResult>>
		{
			return tryPacking(font, float(curFontSize), imageSize, scale, borderSuperSample, characters);
		}, minFont, maxFont, fontSize);
	} else {
		throw Exception("Neither font size nor image size were specified", HalleyExceptions::Tools);
	}
	
	if (!result) {
		throw Exception("Unable to generate font", HalleyExceptions::Tools);
	}
	font.setSize(float(fontSize));
	
	if (!progressReporter(0.1f, "Encoding")) {
		return FontGeneratorResult();
	}

	const auto type = DistanceFieldGenerator::Type::MTSDF;
	auto dstImg = std::make_unique<Image>(type == DistanceFieldGenerator::Type::MTSDF ? Image::Format::RGBA : Image::Format::SingleChannel, imageSize);
	dstImg->clear(0);

	Vector<CharcodeEntry> codes;
	Vector<Future<void>> futures;
	std::mutex m;
	std::atomic<int> nDone(0);
	std::atomic<bool> keepGoing(true);

	auto& pack = result.value();

	for (auto& r : pack) {
		int charcode = int(reinterpret_cast<size_t>(r.data));
		Rect4i dstRect = Rect4i(r.rect.getTopLeft(), r.rect.getBottomRight() - Vector2i(1, 1));
		Rect4i srcRect = dstRect * superSample;
		codes.push_back(CharcodeEntry(charcode, dstRect));

		const bool useMsdfgen = meta.getBool("msdfgen", true);
		if (useMsdfgen) {
			auto finalGlyphImg = DistanceFieldGenerator::generateMSDF(type, font, font.getSize() / superSample, charcode, dstRect.getSize(), radius);
			dstImg->blitFrom(dstRect.getTopLeft(), *finalGlyphImg);

			const float progress = lerp(0.1f, 0.95f, static_cast<float>(++nDone) / static_cast<float>(pack.size()));
			if (!progressReporter(progress, "Generating")) {
				keepGoing = false;
			}
		} else {
			futures.push_back(Concurrent::execute([=, &m, &font, &dstImg, &nDone, &keepGoing] {
				if (!keepGoing) {
					return;
				}

				auto tmpImg = std::make_unique<Image>(Image::Format::RGBA, srcRect.getSize());
				tmpImg->clear(0);
				{
					std::lock_guard<std::mutex> g(m);
					font.drawGlyph(*tmpImg, charcode, Vector2i(lround(borderSuperSample), lround(borderSuperSample)));
				}

				if (!keepGoing) {
					return;
				}
				auto finalGlyphImg = DistanceFieldGenerator::generateSDF(*tmpImg, dstRect.getSize(), radius);
				dstImg->blitFrom(dstRect.getTopLeft(), *finalGlyphImg);

				tmpImg.reset();
				finalGlyphImg.reset();

				const float progress = lerp(0.1f, 0.95f, static_cast<float>(++nDone) / static_cast<float>(pack.size()));
				if (!progressReporter(progress, "Generating")) {
					keepGoing = false;
				}
			}));
		}
	}
	std::sort(codes.begin(), codes.end(), [](const CharcodeEntry& a, const CharcodeEntry& b) { return a.charcode < b.charcode; });

	for (auto& f : futures) {
		f.get();
	}
	if (!keepGoing) {
		return FontGeneratorResult();
	}
	
	if (!progressReporter(0.95f, "Generating files")) {
		return FontGeneratorResult();
	}

	FontGeneratorResult genResult;
	genResult.success = true;
	genResult.font = generateFontMapBinary(meta, font, codes, scale, sizeInfo.replacementScale, radius, imageSize);
	genResult.image = std::move(dstImg);
	genResult.imageMeta = generateTextureMeta();
	progressReporter(1.0f, "Done");

	return genResult;
}

std::unique_ptr<Font> FontGenerator::generateFontMapBinary(const Metadata& meta, FontFace& font, Vector<CharcodeEntry>& entries, float scale, float replacementScale, float radius, Vector2i imageSize) const
{
	String fontName = meta.getString("fontName", font.getName());
	String imageName = "fontTex/" + fontName;

	const float ascender = float(lround(font.getAscender() * scale) + meta.getInt("ascenderAdjustment", 0));
	const float height = float(lround(font.getHeight() * scale) + meta.getInt("lineSpacing", 0));
	const float sizePt = float(lround(font.getSize() * scale));
	const float smoothRadius = radius;
	const int padding = lround(radius);
	const bool floorGlyphPosition = meta.getBool("floorGlyphPosition", false);

	Vector<String> fallback;
	for (auto& name: meta.getString("fallback", "").split(",")) {
		auto trimmedName = name.trimBoth();
		if (!trimmedName.isEmpty()) {
			fallback.push_back(trimmedName);
		}
	}

	std::unique_ptr<Font> result = std::make_unique<Font>(fontName, imageName, ascender, height, sizePt, replacementScale, imageSize, smoothRadius, fallback, floorGlyphPosition);

	Vector<int> charcodes;
	charcodes.reserve(entries.size());
	for (auto& c: entries) {
		charcodes.push_back(c.charcode);
	}
	HashMap<int, HashMap<int, Vector2f>> kerningMap;
	for (const auto& kerningPair: font.getKerning(charcodes)) {
		kerningMap[kerningPair.left][kerningPair.right] = kerningPair.kerning;
	}

	for (auto& c: entries) {
		auto metrics = font.getMetrics(c.charcode, scale);

		const int32_t charcode = c.charcode;
		const Rect4f area = Rect4f(c.rect) / Vector2f(imageSize);
		const Vector2f size = Vector2f(c.rect.getSize());
		const Vector2f horizontalBearing = metrics.bearingHorizontal + Vector2f(float(-padding), float(padding));
		const Vector2f verticalBearing = metrics.bearingVertical + Vector2f(float(-padding), float(padding));
		const Vector2f advance = metrics.advance;

		result->addGlyph(Font::Glyph(charcode, area, size, horizontalBearing, verticalBearing, advance, std::move(kerningMap[charcode])));
	}
	
	return result;
}

std::unique_ptr<Metadata> FontGenerator::generateTextureMeta()
{
	auto meta = std::make_unique<Metadata>();
	meta->set("filtering", true);
	meta->set("mipmap", false);
	meta->set("format", "red");
	meta->set("compression", "raw_image");
	return meta;
}

FontGeneratorResult::FontGeneratorResult() = default;
FontGeneratorResult::FontGeneratorResult(FontGeneratorResult&& other) noexcept = default;
FontGeneratorResult::~FontGeneratorResult() = default;

Vector<Path> FontGeneratorResult::write(Path dir, bool verbose) const
{
	Path fileName = font->getName();
	Path imgName = fileName.replaceExtension(".png");
	Path pngPath = imgName;
	Path binPath = fileName.replaceExtension(".font");
	Path metaPath = pngPath.replaceExtension(".png.meta");

	FileSystem::writeFile(dir / pngPath, image->savePNGToBytes());
	FileSystem::writeFile(dir / binPath, Serializer::toBytes(*font));
	FileSystem::writeFile(dir / metaPath, Serializer::toBytes(*imageMeta));

	return {pngPath, binPath, metaPath};
}
