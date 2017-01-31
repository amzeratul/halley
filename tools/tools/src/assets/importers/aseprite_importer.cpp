#include "aseprite_importer.h"
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

void AsepriteImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	// Meta
	Metadata meta;
	Vector2i pivot;
	if (asset.metadata) {
		meta = *asset.metadata;
		pivot.x = meta.getInt("pivotX", 0);
		pivot.y = meta.getInt("pivotY", 0);
	}

	// Import
	String spriteName = Path(asset.assetId).replaceExtension("").string();
	auto frames = importAseprite(spriteName, gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles[0].data)));

	// Write animation
	Animation animation = generateAnimation(spriteName, frames);
	collector.output(spriteName, AssetType::Animation, Serializer::toBytes(animation));

	// Split grid
	Vector2i grid(meta.getInt("tileWidth", 0), meta.getInt("tileHeight", 0));
	if (grid.x > 0 && grid.y > 0) {
		frames = splitImagesInGrid(frames, grid);
	}

	// Generate atlas + spritesheet
	SpriteSheet spriteSheet;
	auto atlasImage = generateAtlas(spriteName, frames, spriteSheet, pivot);
	collector.output(spriteName, AssetType::SpriteSheet, Serializer::toBytes(spriteSheet));

	// Image metafile
	auto size = atlasImage->getSize();
	meta.set("width", size.x);
	meta.set("height", size.y);

	// Write image
	ImportingAsset image;
	image.assetId = spriteName;
	image.assetType = ImportAssetType::Image;
	image.metadata = std::make_unique<Metadata>(meta);
	image.inputFiles.emplace_back(ImportingAssetFile(spriteName, atlasImage->savePNGToBytes()));
	collector.addAdditionalAsset(std::move(image));
}

std::vector<AsepriteImporter::ImageData> AsepriteImporter::loadImagesFromPath(Path tmp) {
	std::vector<ImageData> frameData;
	for (auto p : FileSystem::enumerateDirectory(tmp)) {
		if (p.getExtension() == ".png") {
			ImageData data;

			auto bytes = FileSystem::readFile(tmp / p);
			data.img = std::make_unique<Image>(p.getFilename().getString(), gsl::as_bytes(gsl::span<Byte>(bytes)), false);
			auto parsedName = p.getStem().getString().split("___");
			if (parsedName.size() != 3 || parsedName[0] != "out") {
				throw Exception("Error parsing filename: " + p.getStem().getString());
			}
			data.sequenceName = parsedName[1];
			data.frameNumber = parsedName[2].toInteger();
			data.clip = data.img->getTrimRect();
			frameData.push_back(std::move(data));
		}
	}
	return frameData;
}

std::map<int, int> AsepriteImporter::getSpriteDurations(Path jsonPath) {
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

void AsepriteImporter::processFrameData(String baseName, std::vector<ImageData>& frameData, std::map<int, int> durations) {
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
		frame.img->setName(frame.filename);
	}
}

std::vector<AsepriteImporter::ImageData> AsepriteImporter::importAseprite(String baseName, gsl::span<const gsl::byte> fileData)
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
	std::vector<ImageData> frameData = loadImagesFromPath(tmp);
	std::map<int, int> durations = getSpriteDurations(jsonPath);

	// Remove temp
	FileSystem::remove(tmp);

	// Process images
	processFrameData(Path(baseName).getFilename().string(), frameData, durations);
	return frameData;
}

Animation AsepriteImporter::generateAnimation(const String& assetName, const std::vector<ImageData>& frameData)
{
	Animation animation;

	animation.setName(assetName);
	animation.setMaterialName("Halley/Sprite");
	animation.setSpriteSheetName(assetName);

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

std::unique_ptr<Image> AsepriteImporter::generateAtlas(const String& assetName, std::vector<ImageData>& images, SpriteSheet& spriteSheet, Vector2i pivot)
{
	// Generate entries
	std::vector<BinPackEntry> entries;
	for (auto& img: images) {
		entries.push_back(BinPackEntry(img.clip.getSize(), &img));
	}

	// Try packing
	int maxSize = 2048;
	int curSize = 32;
	bool wide = false;
	while (curSize < maxSize) {
		Vector2i size(curSize * (wide ? 2 : 1), curSize);
		auto res = BinPack::pack(entries, size);
		if (res.is_initialized()) {
			// Found a pack
			return makeAtlas(assetName, res.get(), size, spriteSheet, pivot);
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

std::unique_ptr<Image> AsepriteImporter::makeAtlas(const String& assetName, const std::vector<BinPackResult>& result, Vector2i size, SpriteSheet& spriteSheet, Vector2i pivot)
{
	auto image = std::make_unique<Image>(size.x, size.y);
	image->clear(0);

	spriteSheet.setTextureName(assetName);

	for (auto& packedImg: result) {
		ImageData* img = reinterpret_cast<ImageData*>(packedImg.data);
		image->blitFrom(packedImg.rect.getTopLeft(), *img->img, img->clip, packedImg.rotated);

		SpriteSheetEntry entry;
		entry.size = Vector2f(img->clip.getSize());
		entry.rotated = packedImg.rotated;
		entry.pivot = Vector2f(pivot - img->clip.getTopLeft()) / entry.size;
		entry.coords = Rect4f(packedImg.rect) / Vector2f(size);
		spriteSheet.addSprite(img->filename, entry);
	}

	return image;
}

std::vector<AsepriteImporter::ImageData> AsepriteImporter::splitImagesInGrid(const std::vector<ImageData>& images, Vector2i grid)
{
	std::vector<ImageData> result;

	for (auto& src: images) {
		auto imgSize = src.img->getSize();
		int nX = imgSize.x / grid.x;
		int nY = imgSize.y / grid.y;

		for (int y = 0; y < nY; ++y) {
			for (int x = 0; x < nX; ++x) {
				auto img = std::make_unique<Image>(grid.x, grid.y);
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
