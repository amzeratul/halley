#include "sprite_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/file_formats/image.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/core/graphics/sprite/animation.h"
#include <sstream>
#include "halley/data_structures/bin_pack.h"
#include "halley/text/string_converter.h"

using namespace Halley;

void SpriteImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	String atlasName = Path(asset.assetId).replaceExtension("").string();
	std::vector<ImageData> totalFrames;

	Maybe<String> palette;

	for (auto& inputFile: asset.inputFiles) {
		// Meta
		Metadata meta = inputFile.metadata;
		Vector2i pivot;
		pivot.x = meta.getInt("pivotX", 0);
		pivot.y = meta.getInt("pivotY", 0);

		// Palette
		auto thisPalette = meta.getString("palette", "");
		if (palette) {
			if (thisPalette != palette.get()) {
				throw Exception("Incompatible palettes in atlas \"" + atlasName + "\". Previously using \"" + palette.get() + "\", now trying to use \"" + thisPalette + "\"");
			}
		} else {
			palette = thisPalette;
		}

		std::vector<ImageData> frames;
		if (inputFile.name.getExtension() == ".ase") {
			// Aseprite
			// Import
			String spriteName = Path(inputFile.name).replaceExtension("").string();
			frames = importAseprite(spriteName, gsl::as_bytes(gsl::span<const Byte>(inputFile.data)), pivot);

			// Write animation
			Animation animation = generateAnimation(spriteName, atlasName, meta.getString("material", "Halley/Sprite"), frames);
			collector.output(spriteName, AssetType::Animation, Serializer::toBytes(animation));
		} else {
			// Bitmap
			auto span = gsl::as_bytes(gsl::span<const Byte>(inputFile.data));
			auto image = std::make_unique<Image>(span, fromString<Image::Format>(meta.getString("format", "undefined")));

			frames.emplace_back();
			auto& imgData = frames.back();
			imgData.pivot = pivot;
			imgData.clip = image->getTrimRect(); // Be careful, make sure this is done before the std::move() below
			imgData.img = std::move(image);
			imgData.duration = 100;
			imgData.filename = inputFile.name.toString();
			imgData.frameNumber = 0;
			imgData.sequenceName = "";
		}

		// Split into a grid
		Vector2i grid(meta.getInt("tileWidth", 0), meta.getInt("tileHeight", 0));
		if (grid.x > 0 && grid.y > 0) {
			frames = splitImagesInGrid(frames, grid);
		}

		std::move(frames.begin(), frames.end(), std::back_inserter(totalFrames));
	}

	// Generate atlas + spritesheet
	SpriteSheet spriteSheet;
	auto atlasImage = generateAtlas(atlasName, totalFrames, spriteSheet);

	// Image metafile
	auto size = atlasImage->getSize();
	Metadata meta;
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
	collector.output(atlasName, AssetType::SpriteSheet, Serializer::toBytes(spriteSheet));
}

std::vector<SpriteImporter::ImageData> SpriteImporter::loadImagesFromPath(Path tmp, Vector2i pivot) {
	std::vector<ImageData> frameData;
	for (auto p : FileSystem::enumerateDirectory(tmp)) {
		if (p.getExtension() == ".png") {
			ImageData data;

			auto bytes = FileSystem::readFile(tmp / p);
			data.img = std::make_unique<Image>(gsl::as_bytes(gsl::span<Byte>(bytes)));
			auto parsedName = p.getStem().getString().split("___");
			if (parsedName.size() != 3 || parsedName[0] != "out") {
				throw Exception("Error parsing filename: " + p.getStem().getString());
			}
			data.sequenceName = parsedName[1];
			data.frameNumber = parsedName[2].toInteger();
			data.clip = data.img->getTrimRect();
			data.pivot = pivot;
			frameData.push_back(std::move(data));
		}
	}
	return frameData;
}

std::map<int, int> SpriteImporter::getSpriteDurations(Path jsonPath) {
	std::map<int, int> durations;
	SpriteSheet spriteSheet;
	auto jsonData = FileSystem::readFile(jsonPath);
	spriteSheet.loadJson(gsl::as_bytes(gsl::span<Byte>(jsonData)));

	// Load durations
	for (auto& name: spriteSheet.getSpriteNames()) {
		auto& sprite = spriteSheet.getSprite(name);
		auto parsedNames = Path(name).getStem().getString().split("___");
		int f = parsedNames.back().toInteger();
		durations[f] = sprite.duration;
	}

	return durations;
}

void SpriteImporter::processFrameData(String baseName, std::vector<ImageData>& frameData, std::map<int, int> durations) {
	std::sort(frameData.begin(), frameData.end(), [] (const ImageData& a, const ImageData& b) -> bool {
		return a.frameNumber < b.frameNumber;
	});

	struct TagInfo {
		int num = 0;
		int cur = 0;
	};

	std::map<String, TagInfo> tags;
	for (auto& frame: frameData) {
		tags[frame.sequenceName].num++;
	}

	for (auto& frame: frameData) {
		auto& tag = tags[frame.sequenceName];
		frame.duration = durations[frame.frameNumber];
		frame.frameNumber = tag.cur++;

		bool hasFrameNumber = tag.num > 1;

		std::stringstream ss;
		ss << baseName.cppStr();
		if (frame.sequenceName != "") {
			ss << "_" << frame.sequenceName.cppStr();
		}
		if (hasFrameNumber) {
			ss << "_" << std::setw(3) << std::setfill('0') << frame.frameNumber;
		}
		frame.filename = ss.str();
	}
}

std::vector<SpriteImporter::ImageData> SpriteImporter::importAseprite(String baseName, gsl::span<const gsl::byte> fileData, Vector2i pivot)
{
	// Make temporary folder
	Path tmp = FileSystem::getTemporaryPath();
	FileSystem::createDir(tmp);
	Path tmpFilePath = tmp / "sprite.ase";
	FileSystem::createParentDir(tmpFilePath);
	FileSystem::writeFile(tmpFilePath, fileData);

	// Run aseprite
	Path jsonPath = tmpFilePath.parentPath() / "data.json";
	Path baseOutputPath = tmpFilePath.parentPath() / "out";
	if (FileSystem::runCommand("aseprite -b " + tmpFilePath.getString() + " --list-tags --data " + jsonPath.getString() + " --filename-format {path}/out___{tag}___{frame000}.png --save-as " + baseOutputPath.getString() + ".png") != 0) {
		throw Exception("Unable to execute aseprite.");
	}

	// Load all images
	std::vector<ImageData> frameData = loadImagesFromPath(tmp, pivot);
	std::map<int, int> durations = getSpriteDurations(jsonPath);

	// Remove temp
	FileSystem::remove(tmp);

	// Process images
	processFrameData(Path(baseName).getFilename().string(), frameData, durations);
	return frameData;
}

Animation SpriteImporter::generateAnimation(const String& spriteName, const String& atlasName, const String& materialName, const std::vector<ImageData>& frameData)
{
	Animation animation;

	animation.setName(spriteName);
	animation.setMaterialName(materialName);
	animation.setSpriteSheetName(atlasName);

	std::map<String, AnimationSequence> sequences;

	animation.addDirection(AnimationDirection("right", "forward", false, 0));
	animation.addDirection(AnimationDirection("left", "forward", true, 0));
	
	for (auto& frame: frameData) {
		auto i = sequences.find(frame.sequenceName);
		if (i == sequences.end()) {
			sequences[frame.sequenceName] = AnimationSequence(frame.sequenceName, 1000.0f / float(frame.duration), true, false);
		}
		auto& seq = sequences[frame.sequenceName];

		seq.addFrame(AnimationFrameDefinition(frame.frameNumber, frame.filename));
	}

	for (auto& seq: sequences) {
		animation.addSequence(seq.second);
	}

	return animation;
}

std::unique_ptr<Image> SpriteImporter::generateAtlas(const String& assetName, std::vector<ImageData>& images, SpriteSheet& spriteSheet)
{
	// Generate entries
	std::vector<BinPackEntry> entries;
	for (auto& img: images) {
		entries.push_back(BinPackEntry(img.clip.getSize(), &img));
	}

	// Try packing
	int maxSize = 4096;
	int curSize = 32;
	bool wide = false;
	while (curSize < maxSize) {
		Vector2i size(curSize * (wide ? 2 : 1), curSize);
		auto res = BinPack::pack(entries, size);
		if (res.is_initialized()) {
			// Found a pack
			return makeAtlas(assetName, res.get(), size, spriteSheet);
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

	throw Exception("Unable to pack sprites in a reasonably sized atlas!");
}

std::unique_ptr<Image> SpriteImporter::makeAtlas(const String& assetName, const std::vector<BinPackResult>& result, Vector2i size, SpriteSheet& spriteSheet)
{
	auto image = std::make_unique<Image>(Image::Format::RGBA, size);
	image->clear(0);

	spriteSheet.setTextureName(assetName);

	for (auto& packedImg: result) {
		ImageData* img = reinterpret_cast<ImageData*>(packedImg.data);
		image->blitFrom(packedImg.rect.getTopLeft(), *img->img, img->clip, packedImg.rotated);

		auto borderTL = img->clip.getTopLeft();
		auto borderBR = img->img->getSize() - img->clip.getSize() - borderTL;

		SpriteSheetEntry entry;
		entry.size = Vector2f(img->clip.getSize());
		entry.rotated = packedImg.rotated;
		entry.pivot = Vector2f(img->pivot - img->clip.getTopLeft()) / entry.size;
		entry.origPivot = img->pivot;
		entry.coords = Rect4f(packedImg.rect) / Vector2f(size);
		entry.trimBorder = Vector4i(borderTL.x, borderTL.y, borderBR.x, borderBR.y);
		spriteSheet.addSprite(img->filename, entry);
	}

	return image;
}

std::vector<SpriteImporter::ImageData> SpriteImporter::splitImagesInGrid(const std::vector<ImageData>& images, Vector2i grid)
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
					result.push_back(ImageData());
					auto& dst = result.back();

					String suffix = "_" + toString(x) + "_" + toString(y);

					dst.duration = src.duration;
					dst.frameNumber = src.frameNumber;
					dst.filename = src.filename + suffix;
					dst.sequenceName = src.sequenceName + suffix;
					dst.img = std::move(img);
					dst.clip = Rect4i({}, grid);
				}
			}
		}
	}

	return result;
}
