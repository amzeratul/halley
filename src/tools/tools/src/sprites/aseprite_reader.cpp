#include "aseprite_reader.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/file/path.h"
#include "halley/file_formats/image.h"
using namespace Halley;

std::vector<ImageData> AsepriteReader::loadImagesFromPath(Path tmp, Vector2i pivot) {
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

std::map<int, int> AsepriteReader::getSpriteDurations(Path jsonPath) {
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

void AsepriteReader::processFrameData(String baseName, std::vector<ImageData>& frameData, std::map<int, int> durations) {
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

std::vector<ImageData> AsepriteReader::importAseprite(String baseName, gsl::span<const gsl::byte> fileData, Vector2i pivot)
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
