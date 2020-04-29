#include "sprite_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/file_formats/image.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/core/graphics/sprite/animation.h"
#include "halley/data_structures/bin_pack.h"
#include "halley/text/string_converter.h"
#include "../../sprites/aseprite_reader.h"
#include "halley/support/logger.h"
#include "halley/utils/hash.h"

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
	String baseSpriteSheetName = Path(asset.assetId).replaceExtension("").string();
	std::map<String, std::vector<ImageData>> totalGroupedFrames;

	std::optional<Metadata> startMeta;

	std::optional<String> palette;

	for (auto& inputFile: asset.inputFiles) {
		auto fileInputId = Path(inputFile.name).dropFront(1);
		const String baseSpriteName = fileInputId.replaceExtension("").string();

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
			if (thisPalette != palette.value()) {
				throw Exception("Incompatible palettes in atlas \"" + asset.assetId + "\". Previously using \"" + palette.value() + "\", now trying to use \"" + thisPalette + "\"", HalleyExceptions::Tools);
			}
		} else {
			palette = thisPalette;
		}

		// Import image data
		std::map<String, std::vector<ImageData>> groupedFrames;
		
		if (inputFile.name.getExtension() == ".ase" || inputFile.name.getExtension() == ".aseprite") {
			// Import Aseprite file
			auto groupSeparated = meta.getBool("group_separated", false);
			groupedFrames = AsepriteReader::importAseprite(baseSpriteName, gsl::as_bytes(gsl::span<const Byte>(inputFile.data)), trim, groupSeparated);
		}
		else {
			// Bitmap
			auto span = gsl::as_bytes(gsl::span<const Byte>(inputFile.data));
			auto image = std::make_unique<Image>(span, fromString<Image::Format>(meta.getString("format", "undefined")));
			
			groupedFrames[""] = std::vector<ImageData>();
			auto& frames = groupedFrames[""];
			auto& imgData = frames.emplace_back();
			imgData.clip = trim ? image->getTrimRect() : image->getRect(); // Be careful, make sure this is done before the std::move() below
			imgData.img = std::move(image);
			imgData.duration = 100;
			imgData.filenames.emplace_back(":img:" + fileInputId.toString());
			imgData.frameNumber = 0;
			imgData.sequenceName = "";
		}

		auto oneAtlas = meta.getString("atlas", "");
		
		for (auto& frames : groupedFrames) {
			// Update frames with pivot and slices
			for (auto& f : frames.second) {
				f.pivot = pivot;
				f.slices = slices;
			}

			// Split into a grid
			const Vector2i grid(meta.getInt("tileWidth", 0), meta.getInt("tileHeight", 0));
			if (grid.x > 0 && grid.y > 0) {
				frames.second = splitImagesInGrid(frames.second, grid);
			}
			
			auto spriteSheetName = baseSpriteSheetName;// +(frames.first.isEmpty() ? "" : ":" + frames.first);
			auto spriteName = baseSpriteName + (frames.first.isEmpty() ? "" : ":" + frames.first);
			Animation animation = generateAnimation(spriteName, spriteSheetName, meta.getString("material", "Halley/Sprite"), frames.second);
			collector.output(spriteName, AssetType::Animation, Serializer::toBytes(animation), {}, "pc", inputFile.name);

			std::vector<ImageData> totalFrames;
			std::move(frames.second.begin(), frames.second.end(), std::back_inserter(totalFrames));
			totalGroupedFrames[spriteSheetName + " " + spriteName] = std::move(totalFrames);
		}
		
	}

	// Generate atlas + spritesheet
	std::vector<ImageData> totalFrames;
	for (auto& frames : totalGroupedFrames)
	{
		std::move(frames.second.begin(), frames.second.end(), std::back_inserter(totalFrames));
	}

	// Metafile
	Metadata meta;
	if (startMeta) {
		meta = startMeta.value();
	}

	// Create the atlas
	auto groupAtlasName = asset.assetId;
	SpriteSheet spriteSheet;
	auto atlasImage = generateAtlas(groupAtlasName, totalFrames, spriteSheet);
	spriteSheet.setTextureName(groupAtlasName);
	spriteSheet.setDefaultMaterialName(meta.getString("defaultMaterial", "Halley/Sprite"));

	// Metafile parameters
	auto size = atlasImage->getSize();
	if (palette) {
		meta.set("palette", palette.value());
	}
	meta.set("width", size.x);
	meta.set("height", size.y);
	meta.set("compression", "raw_image");

	// Write atlas image
	ImportingAsset image;
	image.assetId = groupAtlasName;
	image.assetType = ImportAssetType::Image;
	image.inputFiles.emplace_back(ImportingAssetFile(groupAtlasName, Serializer::toBytes(*atlasImage), meta));
	collector.addAdditionalAsset(std::move(image));

	// Write spritesheet
	collector.output(baseSpriteSheetName, AssetType::SpriteSheet, Serializer::toBytes(spriteSheet));
}

String SpriteImporter::getAssetId(const Path& file, const std::optional<Metadata>& metadata) const
{
	if (metadata) {
		String atlas = metadata->getString("atlas", "");
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

	// Generate directions
	int numExplicitDirs = 0;
	{
		std::set<String> directionNames;
		for (const auto& frame: frameData) {
			if (!frame.direction.isEmpty()) {
				if (directionNames.find(frame.direction) == directionNames.end()) {
					directionNames.insert(frame.direction);
					numExplicitDirs++;
				}
			}
		}

		if (numExplicitDirs == 0) {
			directionNames.insert("right");
		}

		int i = 0;
		for (const auto& dir: directionNames) {
			animation.addDirection(AnimationDirection(dir, dir, false, i++));
		}

		auto hasAnim = [&](const String& name)
		{
			return directionNames.find(name) != directionNames.end();
		};
		auto replaceAnim = [&](const String& toAdd, const String& base)
		{
			if (!hasAnim(toAdd) && hasAnim(base)) {
				animation.addDirection(AnimationDirection(toAdd, base, true, animation.getDirection(base).getId()));
			}
		};

		replaceAnim("left", "right");
		replaceAnim("up_left", "up_right");
		replaceAnim("down_left", "down_right");
	}
		
	std::map<String, AnimationSequence> sequences;
	std::map<String, std::set<String>> directionsPerSequence;

	for (const auto& frame : frameData) {
		String sequence = frame.sequenceName;

		auto i = sequences.find(sequence);
		if (i == sequences.end()) {
			sequences[sequence] = AnimationSequence(sequence, true, false);
		}

		directionsPerSequence[sequence].insert(frame.direction);
	}

	for (const auto& frame : frameData) {
		String sequence = frame.sequenceName;
		auto& seq = sequences[sequence];
		if (static_cast<int>(seq.numFrameDefinitions()) == frame.frameNumber) {
			auto filename = frame.filenames.at(0);
			if (!directionsPerSequence.at(sequence).empty() && !frame.direction.isEmpty()) {
				filename = filename.replaceAll("_" + frame.direction, "_%dir%");
			}
			seq.addFrame(AnimationFrameDefinition(frame.frameNumber, frame.duration, filename));
		}
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

	markDuplicates(images);

	// Generate entries
	int64_t totalImageArea = 0;
	std::vector<BinPackEntry> entries;
	entries.reserve(images.size());
	for (auto& img: images) {
		if (!img.isDuplicate) {
			auto size = img.clip.getSize();
			totalImageArea += size.x * size.y;
			entries.emplace_back(size, &img);
		}
	}

	// Figure out a reasonable pack size to start with
	const int minSize = nextPowerOf2(static_cast<int>(sqrt(static_cast<double>(totalImageArea)))) / 2;
	const int64_t guessArea = int64_t(minSize) * int64_t(minSize);
	const int maxSize = 4096;
	int curSize = std::min(maxSize, std::max(32, static_cast<int>(minSize)));

	// Try packing
	bool wide = guessArea > 2 * totalImageArea;
	while (true) {
		Vector2i size(curSize * (wide ? 2 : 1), curSize);
		if (size.x > maxSize || size.y > maxSize) {
			// Give up!
			throw Exception("Unable to pack " + toString(images.size()) + " sprites in a reasonably sized atlas! curSize at " + toString(curSize) + ", maxSize is " + toString(maxSize) + ". Total image area is " + toString(totalImageArea) + " px^2, sqrt = " + toString(lround(sqrt(totalImageArea))) + " px.", HalleyExceptions::Tools);
		}

		Logger::logInfo("Trying " + toString(size.x) + "x" + toString(size.y) + " px...");
		auto res = BinPack::fastPack(entries, size);
		if (res) {
			// Found a pack
			if (images.size() > 1) {
				Logger::logInfo("Atlas \"" + atlasName + "\" generated at " + toString(size.x) + "x" + toString(size.y) + " px with " + toString(images.size()) + " sprites. Total image area is " + toString(totalImageArea) + " px^2, sqrt = " + toString(lround(sqrt(totalImageArea))) + " px.");
			}
			
			return makeAtlas(res.value(), spriteSheet);
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
}

std::unique_ptr<Image> SpriteImporter::makeAtlas(const std::vector<BinPackResult>& result, SpriteSheet& spriteSheet)
{
	Vector2i size = computeAtlasSize(result);

	std::unique_ptr<Image> atlasImage;

	for (const auto& packedImg: result) {
		const ImageData* img = reinterpret_cast<ImageData*>(packedImg.data);

		if (!atlasImage) {
			atlasImage = std::make_unique<Image>(img->img->getFormat(), size);
			atlasImage->clear(0);
		}
		if (atlasImage->getFormat() != img->img->getFormat()) {
			throw Exception("Mixed image formats in atlas.", HalleyExceptions::Tools);
		}
		
		atlasImage->blitFrom(packedImg.rect.getTopLeft(), *img->img, img->clip, packedImg.rotated);

		const auto borderTL = img->clip.getTopLeft();
		const auto borderBR = img->img->getSize() - img->clip.getSize() - borderTL;

		// HACK: Hmmm, suspicious
		const auto offset = Vector2f(0.0001f, 0.0001f);

		auto addImageData = [&] (const ImageData& imgData) {
			SpriteSheetEntry entry;
			entry.size = Vector2f(imgData.clip.getSize());
			entry.rotated = packedImg.rotated;
			entry.pivot = Vector2f(imgData.pivot - imgData.clip.getTopLeft()) / entry.size;
			entry.origPivot = imgData.pivot;
			entry.coords = (Rect4f(Vector2f(packedImg.rect.getTopLeft()) + offset, Vector2f(packedImg.rect.getBottomRight()) + offset)) / Vector2f(size);
			entry.trimBorder = Vector4s(static_cast<short>(borderTL.x), static_cast<short>(borderTL.y), static_cast<short>(borderBR.x), static_cast<short>(borderBR.y));
			entry.slices = imgData.slices;

			for (const auto& filename: imgData.filenames) {
				spriteSheet.addSprite(filename, entry);
			}
		};
		
		addImageData(*img);
		for (const auto& dupe: img->duplicatesOfThis) {
			addImageData(*dupe);
		}
	}

	return atlasImage;
}

Vector2i SpriteImporter::computeAtlasSize(const std::vector<BinPackResult>& results) const
{
	int w = 0;
	int h = 0;

	for (const auto& r: results) {
		w = std::max(w, r.rect.getRight());
		h = std::max(h, r.rect.getBottom());
	}

	return Vector2i(nextPowerOf2(w), nextPowerOf2(h));
}

std::vector<ImageData> SpriteImporter::splitImagesInGrid(const std::vector<ImageData>& images, Vector2i grid)
{
	std::vector<ImageData> result;

	for (const auto& src: images) {
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

void SpriteImporter::markDuplicates(std::vector<ImageData>& images) const
{
	std::unordered_map<uint64_t, ImageData*> hashes;
	
	for (auto& image: images) {
		Hash::Hasher hasher;
		const auto clip = image.clip;
		if (clip.isEmpty()) {
			continue;
		}
		for (int y = clip.getTop(); y < clip.getBottom(); ++y) {
			const auto row = image.img->getPixelBytesRow(clip.getLeft(), clip.getRight(), y);
			hasher.feedBytes(as_bytes(row));
		}
		const auto hash = hasher.digest();

		const auto iter = hashes.find(hash);
		if (iter == hashes.end()) {
			hashes[hash] = &image;
		} else {
			image.isDuplicate = true;
			iter->second->duplicatesOfThis.push_back(&image);
		}
	}
}
