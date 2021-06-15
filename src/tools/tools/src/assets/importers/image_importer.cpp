#include "image_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/file_formats/image.h"
#include "halley/tools/file/filesystem.h"
#include "halley/maths/colour.h"

using namespace Halley;

void ImageImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	auto& input = asset.inputFiles.at(0);

	// Prepare metadata
	Metadata meta = input.metadata;

	// Load image
	Path mainFile = asset.inputFiles.at(0).name;
	auto span = gsl::as_bytes(gsl::span<const Byte>(input.data));
	std::unique_ptr<Image> image;
	if (meta.getString("compression", "png") == "png") {
		image = std::make_unique<Image>(span, fromString<Image::Format>(meta.getString("format", "undefined")));
	} else {
		image = std::make_unique<Image>();
		Deserializer s(span);
		s >> *image;
	}

	// Convert to indexed mode
	auto palette = meta.getString("palette", "");
	bool paletteTopLineOnly = meta.getBool("paletteTopLineOnly", false);
	if (palette != "") {
		auto paletteBytes = collector.readAdditionalFile(palette);
		Image paletteImage(gsl::as_bytes(gsl::span<Byte>(paletteBytes)));
		image = convertToIndexed(mainFile.string(), *image, paletteImage, asset.options, paletteTopLineOnly);
	}

	// Fill meta
	Vector2i size = image->getSize();
	meta.set("width", size.x);
	meta.set("height", size.y);
	meta.set("format", toString(image->getFormat()));

	// Output
	ImportingAsset imageAsset;
	imageAsset.assetId = asset.assetId;
	imageAsset.assetType = ImportAssetType::Texture;
	imageAsset.inputFiles.emplace_back(ImportingAssetFile(asset.assetId, Serializer::toBytes(*image), meta));
	collector.addAdditionalAsset(std::move(imageAsset));
}

std::unique_ptr<Image> ImageImporter::convertToIndexed(const String& fileName, const Image& image, const Image& palette, const ConfigNode& assetOptions, bool readTopLineOnly)
{
	auto lookup = makePaletteConversion(palette, readTopLineOnly);

	auto result = std::make_unique<Image>(Image::Format::Indexed, image.getSize());
	auto dst = result->getPixelBytes();
	auto src = image.getPixels4BPP();
	size_t n = image.getWidth() * image.getHeight();

	std::vector<int> coloursMissing;
	std::vector<Vector2i> colourMissingPos;

	for (size_t i = 0; i < n; ++i) {
		auto res = lookup.find(src[i]);
		if (res == lookup.end()) {
			if (std::find(coloursMissing.begin(), coloursMissing.end(), src[i]) == coloursMissing.end()) {
				const auto pos = Vector2i(static_cast<int>(i % image.getWidth()), static_cast<int>(i / image.getWidth()));
				coloursMissing.push_back(src[i]);
				colourMissingPos.push_back(pos);
			}
		} else {
			dst[i] = uint8_t(res->second);
		}
	}

	if (!coloursMissing.empty()) {
		String message = "Colours missing from the palette:";
		const size_t nMissing = coloursMissing.size();
		for (size_t i = 0; i < nMissing; ++i) {
			unsigned int r, g, b, a;
			Image::convertIntToRGBA(coloursMissing[i], r, g, b, a);
			auto pos = lookupSpritePosition(fileName, colourMissingPos[i], assetOptions);
			
			message += "\n\t" + Colour4<int>(r, g, b, a).toString() + " on " + pos.first + ", " + pos.second;
		}
		throw Exception(message, HalleyExceptions::Tools);
	}

	return result;
}

std::unordered_map<uint32_t, uint32_t> ImageImporter::makePaletteConversion(const Image& palette, bool readTopLineOnly)
{
	auto src = palette.getPixels4BPP();
	size_t w = palette.getWidth();
	size_t h = readTopLineOnly ? 1 : palette.getHeight();

	std::unordered_map<uint32_t, uint32_t> lookup;
	for (size_t y = 0; y < h; ++y) {
		for (size_t x = 0; x < w; ++x) {
			auto colour = src[y * w + x];

			auto res = lookup.find(colour);
			if (res == lookup.end()) {
				lookup[colour] = uint32_t(x);
			} else if (colour != 0) {
				unsigned int r, g, b, a;
				Image::convertIntToRGBA(colour, r, g, b, a);
				throw Exception("Colour " + Colour4<int>(r, g, b, a).toString()
					+ " is duplicated in the palette. Found at " + toString(x) + ", " + toString(y) + "; previously found at index "
					+ toString(res->second), HalleyExceptions::Tools);
			}
		}
	}
	return lookup;
}

std::pair<String, Vector2i> ImageImporter::lookupSpritePosition(const String& fileName, Vector2i pos, const ConfigNode& options)
{
	if (!options.hasKey("sprites") || options["sprites"].getType() != ConfigNodeType::Sequence) {
		return { fileName, pos };
	}
	const auto& sprites = options["sprites"].asSequence();
	for (const auto& spriteEntry: sprites) {
		const int x = spriteEntry["x"].asInt();
		const int y = spriteEntry["y"].asInt();
		const int w = spriteEntry["w"].asInt();
		const int h = spriteEntry["h"].asInt();
		if (Rect4i(x, y, w, h).contains(pos)) {
			auto relPos = pos - Vector2i(x, y);
			const bool rotated = spriteEntry["rotated"].asBool();
			if (rotated) {
				std::swap(relPos.x, relPos.y);
			}
			relPos += Vector2i(spriteEntry["offX"].asInt(), spriteEntry["offY"].asInt());

			return { "\"" + spriteEntry["origFilename"].asString() + "\", frame " + toString(spriteEntry["origFrameN"].asInt() + 1) , relPos };
		}
	}

	return { fileName + " [unknown]", pos };
}
