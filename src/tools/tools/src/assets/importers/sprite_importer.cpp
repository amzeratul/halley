#include "sprite_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/file_formats/image.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/core/graphics/sprite/animation.h"
#include "halley/data_structures/bin_pack.h"
#include "halley/text/string_converter.h"
#include "../../sprites/aseprite_reader.h"
#include "halley/support/logger.h"

using namespace Halley;

bool ImageData::operator==(const ImageData& other) const
{
	return frameNumber == other.frameNumber
		&& duration == other.duration
		&& sequenceName == other.sequenceName
		&& clip == other.clip
		&& pivot == other.pivot
		&& slices == other.slices
		&& filenames == other.filenames
		&& img->getSize() == other.img->getSize();
}

bool ImageData::operator!=(const ImageData& other) const
{
	return !(*this == other);
}

void SpriteImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	String atlasName = asset.assetId;
	String spriteSheetName = Path(asset.assetId).replaceExtension("").string();
	std::vector<ImageData> totalFrames;

	Maybe<Metadata> startMeta;

	Maybe<String> palette;

	for (auto& inputFile: asset.inputFiles) {
		auto fileInputId = Path(inputFile.name).dropFront(1);
		const String spriteName = fileInputId.replaceExtension("").string();

		// Meta
		Metadata meta = inputFile.metadata;
		if (!startMeta) {
			startMeta = meta;
		}
		Vector2i pivot;
		pivot.x = meta.getInt("pivotX", 0);
		pivot.y = meta.getInt("pivotY", 0);
		Vector4s slices;
		slices.x = gsl::narrow<short, int>(meta.getInt("slice_left", 0));
		slices.y = gsl::narrow<short, int>(meta.getInt("slice_top", 0));
		slices.z = gsl::narrow<short, int>(meta.getInt("slice_right", 0));
		slices.w = gsl::narrow<short, int>(meta.getInt("slice_bottom", 0));
		bool trim = meta.getBool("trim", true);

		// Palette
		auto thisPalette = meta.getString("palette", "");
		if (palette) {
			if (thisPalette != palette.get()) {
				throw Exception("Incompatible palettes in atlas \"" + atlasName + "\". Previously using \"" + palette.get() + "\", now trying to use \"" + thisPalette + "\"");
			}
		} else {
			palette = thisPalette;
		}

		// Import image data
		std::vector<ImageData> frames;
		if (inputFile.name.getExtension() == ".ase") {
			// Import Aseprite file
			frames = AsepriteReader::importAseprite(spriteName, gsl::as_bytes(gsl::span<const Byte>(inputFile.data)), trim);
		} else {
			// Bitmap
			auto span = gsl::as_bytes(gsl::span<const Byte>(inputFile.data));
			auto image = std::make_unique<Image>(span, fromString<Image::Format>(meta.getString("format", "undefined")));

			frames.emplace_back();
			auto& imgData = frames.back();
			imgData.clip = trim ? image->getTrimRect() : image->getRect(); // Be careful, make sure this is done before the std::move() below
			imgData.img = std::move(image);
			imgData.duration = 100;
			imgData.filenames.emplace_back(":img:" + fileInputId.toString());
			imgData.frameNumber = 0;
			imgData.sequenceName = "";
		}

		// Update frames with pivot and slices
		for (auto& f: frames) {
			f.pivot = pivot;
			f.slices = slices;
		}

		// Split into a grid
		const Vector2i grid(meta.getInt("tileWidth", 0), meta.getInt("tileHeight", 0));
		if (grid.x > 0 && grid.y > 0) {
			frames = splitImagesInGrid(frames, grid);
		}

		// Write animation
		Animation animation = generateAnimation(spriteName, spriteSheetName, meta.getString("material", "Halley/Sprite"), frames);
		collector.output(spriteName, AssetType::Animation, Serializer::toBytes(animation));

		std::move(frames.begin(), frames.end(), std::back_inserter(totalFrames));
	}

	// Generate atlas + spritesheet
	SpriteSheet spriteSheet;
	auto atlasImage = generateAtlas(atlasName, totalFrames, spriteSheet);
	spriteSheet.setTextureName(atlasName);

	// Image metafile
	auto size = atlasImage->getSize();
	Metadata meta;
	if (startMeta) {
		meta = startMeta.get();
	}
	if (palette) {
		meta.set("palette", palette.get());
	}
	meta.set("width", size.x);
	meta.set("height", size.y);
	meta.set("compression", "raw_image");

	// Write atlas image
	ImportingAsset image;
	image.assetId = atlasName;
	image.assetType = ImportAssetType::Image;
	image.inputFiles.emplace_back(ImportingAssetFile(atlasName, Serializer::toBytes(*atlasImage), meta));
	collector.addAdditionalAsset(std::move(image));

	// Write spritesheet
	collector.output(spriteSheetName, AssetType::SpriteSheet, Serializer::toBytes(spriteSheet));
}

String SpriteImporter::getAssetId(const Path& file, const Maybe<Metadata>& metadata) const
{
	if (metadata) {
		String atlas = metadata.get().getString("atlas", "");
		if (atlas != "") {
			return atlas;
		}
	}
	return IAssetImporter::getAssetId(file, metadata);
}

Animation SpriteImporter::generateAnimation(const String& spriteName, const String& spriteSheetName, const String& materialName, const std::vector<ImageData>& frameData)
{
	Animation animation;

	animation.setName(spriteName);
	animation.setMaterialName(materialName);
	animation.setSpriteSheetName(spriteSheetName);

	std::map<String, AnimationSequence> sequences;

	animation.addDirection(AnimationDirection("right", "forward", false, 0));
	animation.addDirection(AnimationDirection("left", "forward", true, 0));
	
	for (auto& frame: frameData) {
		auto i = sequences.find(frame.sequenceName);
		if (i == sequences.end()) {
			sequences[frame.sequenceName] = AnimationSequence(frame.sequenceName, true, false);
		}
		auto& seq = sequences[frame.sequenceName];

		seq.addFrame(AnimationFrameDefinition(frame.frameNumber, frame.duration, frame.filenames.at(0)));
	}

	for (auto& seq: sequences) {
		animation.addSequence(seq.second);
	}

	return animation;
}

std::unique_ptr<Image> SpriteImporter::generateAtlas(const String& atlasName, std::vector<ImageData>& images, SpriteSheet& spriteSheet)
{
	if (images.size() > 1) {
		Logger::logInfo("Generating atlas \"" + atlasName + "\" with " + toString(images.size()) + " sprites...");
	}

	// Generate entries
	int64_t totalImageArea = 0;
	std::vector<BinPackEntry> entries;
	entries.reserve(images.size());
	for (auto& img: images) {
		auto size = img.clip.getSize();
		totalImageArea += size.x * size.y;
		entries.emplace_back(size, &img);
	}

	// Figure out a reasonable pack size to start with
	const int minSize = nextPowerOf2(int(sqrt(double(totalImageArea))));
	const int64_t guessArea = int64_t(minSize) * int64_t(minSize);
	const int maxSize = 4096;
	int curSize = std::min(maxSize, std::max(32, int(minSize)));

	// Try packing
	bool wide = guessArea > 2 * totalImageArea;
	while (curSize < maxSize) {
		Vector2i size(curSize * (wide ? 2 : 1), curSize);
		Logger::logInfo("Trying " + toString(size.x) + "x" + toString(size.y) + " px...");
		auto res = BinPack::pack(entries, size);
		if (res.is_initialized()) {
			// Found a pack
			if (images.size() > 1) {
				Logger::logInfo("Atlas \"" + atlasName + "\" generated at " + toString(size.x) + "x" + toString(size.y) + " px.");
			}

			return makeAtlas(res.get(), size, spriteSheet);
		} else {
			// Try 64x64, then 128x64, 128x128, 256x128, etc
			if (wide) {
				wide = false;
				curSize *= 2;
			} else {
				wide = true;
			}
		}
	}

	throw Exception("Unable to pack " + toString(images.size()) + " sprites in a reasonably sized atlas! curSize at " + toString(curSize) + ", maxSize is " + toString(maxSize) + ". Total image area is " + toString(totalImageArea) + " px^2, sqrt = " + toString(lround(sqrt(totalImageArea))) + " px.");
}

std::unique_ptr<Image> SpriteImporter::makeAtlas(const std::vector<BinPackResult>& result, Vector2i origSize, SpriteSheet& spriteSheet)
{
	Vector2i size = shrinkAtlas(result);

	auto image = std::make_unique<Image>(Image::Format::RGBA, size);
	image->clear(0);

	for (auto& packedImg: result) {
		ImageData* img = reinterpret_cast<ImageData*>(packedImg.data);
		image->blitFrom(packedImg.rect.getTopLeft(), *img->img, img->clip, packedImg.rotated);

		const auto borderTL = img->clip.getTopLeft();
		const auto borderBR = img->img->getSize() - img->clip.getSize() - borderTL;

		const auto offset = Vector2f(0.0001f, 0.0001f);

		SpriteSheetEntry entry;
		entry.size = Vector2f(img->clip.getSize());
		entry.rotated = packedImg.rotated;
		entry.pivot = Vector2f(img->pivot - img->clip.getTopLeft()) / entry.size;
		entry.origPivot = img->pivot;
		entry.coords = (Rect4f(Vector2f(packedImg.rect.getTopLeft()) + offset, Vector2f(packedImg.rect.getBottomRight()) + offset)) / Vector2f(size);
		entry.trimBorder = Vector4s(short(borderTL.x), short(borderTL.y), short(borderBR.x), short(borderBR.y));
		entry.slices = img->slices;

		for (auto& filename: img->filenames) {
			spriteSheet.addSprite(filename, entry);
		}
	}

	return image;
}

Vector2i SpriteImporter::shrinkAtlas(const std::vector<BinPackResult>& results) const
{
	int w = 0;
	int h = 0;

	for (auto& r: results) {
		w = std::max(w, r.rect.getRight());
		h = std::max(h, r.rect.getBottom());
	}

	return Vector2i(nextPowerOf2(w), nextPowerOf2(h));
}

std::vector<ImageData> SpriteImporter::splitImagesInGrid(const std::vector<ImageData>& images, Vector2i grid)
{
	std::vector<ImageData> result;

	for (auto& src: images) {
		auto imgSize = src.img->getSize();
		int nX = imgSize.x / grid.x;
		int nY = imgSize.y / grid.y;

		for (int y = 0; y < nY; ++y) {
			for (int x = 0; x < nX; ++x) {
				auto img = std::make_unique<Image>(Image::Format::RGBA, grid);
				img->blitFrom(Vector2i(), *src.img, Rect4i(Vector2i(x, y) * grid, grid.x, grid.y));
				Rect4i trimRect = img->getTrimRect();
				if (trimRect.getWidth() > 0 && trimRect.getHeight() > 0) {
					result.emplace_back();
					auto& dst = result.back();

					String suffix = "_" + toString(x) + "_" + toString(y);

					dst.duration = src.duration;
					dst.frameNumber = src.frameNumber;
					dst.filenames.emplace_back(src.filenames.at(0) + suffix);
					dst.sequenceName = src.sequenceName + suffix;
					dst.img = std::move(img);
					dst.clip = Rect4i({}, grid);
				}
			}
		}
	}

	return result;
}
