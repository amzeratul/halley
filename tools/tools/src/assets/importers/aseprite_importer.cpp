#include "aseprite_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/file_formats/image.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/core/graphics/sprite/animation.h"
#include <sstream>

using namespace Halley;

std::vector<Path> AsepriteImporter::import(const ImportingAsset& asset, Path dstDir, ProgressReporter reporter, AssetCollector collector)
{
	Path srcPath = asset.inputFiles.at(0).name;
	String baseName = srcPath.getStem().getString();
	Path animationPath = dstDir / "animation" / baseName;
	Path spriteSheetPath = dstDir / "spritesheet" / baseName;
	Path imagePath = dstDir / "image" / (baseName + ".png");

	// Import
	auto frames = importAseprite(baseName, gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles[0].data)));

	// Write animation
	Animation animation = generateAnimation(baseName, frames);
	FileSystem::writeFile(animationPath, Serializer::toBytes(animation));

	// Generate atlas + spritesheet
	Image atlasImage;
	SpriteSheet spriteSheet;
	generateAtlas(baseName, frames, atlasImage, spriteSheet);
	FileSystem::writeFile(imagePath, atlasImage.savePNGToBytes());
	FileSystem::writeFile(spriteSheetPath, Serializer::toBytes(spriteSheet));

	// Image metafile
	Metadata meta;
	if (asset.metadata) {
		meta = *asset.metadata;
	}
	auto size = atlasImage.getSize();
	meta.set("width", size.x);
	meta.set("height", size.y);
	Path imageMetaPath = imagePath.replaceExtension(imagePath.getExtension() + ".meta");
	FileSystem::writeFile(dstDir / imageMetaPath, Serializer::toBytes(meta));

	return { spriteSheetPath, animationPath, imagePath, imageMetaPath };
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
	Path jsonPath = (tmpFilePath.parentPath() / "data.json");
	Path baseOutputPath = (tmpFilePath.parentPath() / "out");
	if (FileSystem::runCommand("aseprite -b " + tmpFilePath.getString() + " --list-tags --data " + jsonPath.getString() + " --filename-format {path}/out___{tag}___{frame000}.png --save-as " + baseOutputPath.getString() + ".png") != 0) {
		throw Exception("Unable to execute aseprite.");
	}

	// Load all images
	std::vector<ImageData> frameData;
	for (auto p : FileSystem::enumerateDirectory(tmp)) {
		if (p.getExtension() == ".png") {
			ImageData data;

			auto bytes = FileSystem::readFile(p);
			data.img = std::make_unique<Image>(p.getFilename().getString(), gsl::as_bytes(gsl::span<Byte>(bytes)), false);
			auto parsedName = p.getStem().getString().split("___");
			if (parsedName.size() != 3 || parsedName[0] != "out") {
				throw Exception("Error parsing filename: " + p.getStem().getString());
			}
			data.sequenceName = parsedName[1];
			data.frameNumber = parsedName[2].toInteger();
			frameData.push_back(std::move(data));
		}
	}

	// Load spritesheet
	SpriteSheet spriteSheet;
	auto jsonData = FileSystem::readFile(jsonPath);
	spriteSheet.loadJson(gsl::as_bytes(gsl::span<Byte>(jsonData)));

	// Remove temp
	FileSystem::remove(tmp);

	// Load durations
	std::map<int, int> durations;
	for (auto& name: spriteSheet.getSpriteNames()) {
		auto& sprite = spriteSheet.getSprite(name);
		auto parsedNames = Path(name).getStem().getString().split("___");
		int f = parsedNames.back().toInteger();
		durations[f] = sprite.duration;
	}

	// Process images
	std::sort(frameData.begin(), frameData.end(), [] (const ImageData& a, const ImageData& b) -> bool {
		return a.frameNumber < b.frameNumber;
	});
	for (auto tag: spriteSheet.getFrameTags()) {
		for (auto& frame: frameData) {
			if (frame.sequenceName == tag.name) {
				frame.duration = durations[frame.frameNumber];
				frame.frameNumber -= tag.from;

				std::stringstream ss;
				ss << baseName.cppStr() << "_" << frame.sequenceName.cppStr() << "_" << std::setw(3) << std::setfill('0') << frame.frameNumber << ".png";
				frame.filename = ss.str();
				frame.img->setName(frame.filename);
			}
		}
	}
	return frameData;
}

Animation AsepriteImporter::generateAnimation(String baseName, const std::vector<ImageData>& frameData)
{
	Animation animation;

	animation.setName(baseName);
	animation.setMaterialName("sprite");
	animation.setSpriteSheetName(baseName);

	std::map<String, AnimationSequence> sequences;

	animation.addDirection(AnimationDirection("right", "forward", false, 0));
	animation.addDirection(AnimationDirection("left", "forward", true, 0));
	
	for (auto& frame: frameData) {
		auto i = sequences.find(frame.sequenceName);
		if (i == sequences.end()) {
			sequences[frame.sequenceName] = AnimationSequence(frame.sequenceName, int(std::round(1000.0f / frame.duration)), true, false);
		}
		auto& seq = sequences[frame.sequenceName];

		seq.addFrame(AnimationFrameDefinition(frame.frameNumber, frame.filename));
	}

	for (auto& seq: sequences) {
		animation.addSequence(seq.second);
	}

	return animation;
}

void AsepriteImporter::generateAtlas(String baseName, const std::vector<ImageData>& images, Image& atlasImage, SpriteSheet& spriteSheet)
{
	// TODO
}
