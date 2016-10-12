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

	std::vector<std::unique_ptr<Image>> frames;
	Animation animation;
	importAseprite(srcPath.getStem().getString(), gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles[0].data)), frames, animation);

	// Generate atlas + spritesheet
	// TODO

	// Generate animation
	// TODO

	Path spriteSheetPath;
	Path imagePath = srcPath;
	Path animationPath;

	// Image metafile
	Metadata meta;
	if (asset.metadata) {
		meta = *asset.metadata;
	}
	//meta.set("width", size.x);
	//meta.set("height", size.y);
	Path imageMetaPath = imagePath.replaceExtension(imagePath.getExtension() + ".meta");
	FileSystem::writeFile(dstDir / imageMetaPath, Serializer::toBytes(meta));

	return { spriteSheetPath, animationPath, imagePath, imageMetaPath };
}

void AsepriteImporter::importAseprite(String baseName, gsl::span<const gsl::byte> fileData, std::vector<std::unique_ptr<Image>>& frames, Animation& animation)
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
	struct ImageData
	{
		int frameNumber;
		int duration;
		String sequenceName;
		String filename;
		std::unique_ptr<Image> img;
	};
	std::vector<ImageData> framesTemp;
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
			framesTemp.push_back(std::move(data));
		}
	}
	std::sort(framesTemp.begin(), framesTemp.end(), [] (const ImageData& a, const ImageData& b) -> bool {
		return a.frameNumber < b.frameNumber;
	});

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
	for (auto tag: spriteSheet.getFrameTags()) {
		for (auto& frame: framesTemp) {
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
	for (auto& frame: framesTemp) {
		frames.push_back(std::move(frame.img));
	}

	// Generate animation

	// 
}
