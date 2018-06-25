#include "aseprite_reader.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/file/path.h"
#include "halley/file_formats/image.h"
#include "../assets/importers/sprite_importer.h"
#include "halley/support/logger.h"
#include "aseprite_file.h"
using namespace Halley;

std::vector<ImageData> AsepriteExternalReader::loadImagesFromPath(Path tmp, bool trim) {
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
			data.clip = trim ? data.img->getTrimRect() : data.img->getRect();
			frameData.push_back(std::move(data));
		}
	}
	return frameData;
}

std::map<int, int> AsepriteExternalReader::getSpriteDurations(Path jsonPath) {
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

void AsepriteExternalReader::processFrameData(String spriteName, std::vector<ImageData>& frameData, std::map<int, int> durations) {
	String baseName = Path(spriteName).getFilename().string();

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
		int origFrameNumber = frame.frameNumber;

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

		frame.filenames.emplace_back(ss.str());
		if (origFrameNumber == 0) {
			frame.filenames.emplace_back(":img:" + spriteName);
		}
	}
}

std::vector<ImageData> AsepriteExternalReader::importAseprite(String spriteName, gsl::span<const gsl::byte> fileData, bool trim)
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
	std::vector<ImageData> frameData = loadImagesFromPath(tmp, trim);
	std::map<int, int> durations = getSpriteDurations(jsonPath);

	// Remove temp
	FileSystem::remove(tmp);

	// Process images
	processFrameData(spriteName, frameData, durations);
	return frameData;
}


////////////


std::vector<ImageData> AsepriteReader::importAseprite(String spriteName, gsl::span<const gsl::byte> fileData, bool trim)
{
	const String baseName = Path(spriteName).getFilename().string();

	AsepriteFile aseFile;
	aseFile.load(fileData);

	const size_t nFrames = aseFile.getNumberOfFrames();
	std::vector<char> frameTagged(nFrames, 0);

	// Load all tags
	std::vector<std::pair<String, std::vector<int>>> tags;
	for (auto& tag: aseFile.getTags()) {
		std::vector<int> frames;
		for (int i = tag.fromFrame; i <= tag.toFrame; ++i) {
			frameTagged[i] = 1;
			frames.push_back(i);
		}
		tags.emplace_back(tag.name, std::move(frames));
	}

	// Create empty tag
	{
		std::vector<int> untagged;
		for (int i = 0; i < int(frameTagged.size()); ++i) {
			if (frameTagged[i] == 0) {
				untagged.push_back(i);
			}
		}
		if (!untagged.empty()) {
			tags.emplace_back("", std::move(untagged));
		}
	}

	// Create frames
	std::vector<ImageData> frameData;
	for (auto& t: tags) {
		int i = 0;
		for (auto& frameN: t.second) {
			frameData.push_back(ImageData());
			auto& imgData = frameData.back();

			imgData.img = aseFile.makeFrameImage(frameN);
			imgData.frameNumber = i;
			imgData.sequenceName = t.first;
			imgData.duration = aseFile.getFrame(frameN).duration;
			imgData.clip = trim ? imgData.img->getTrimRect() : imgData.img->getRect();

			std::stringstream ss;
			ss << baseName.cppStr();
			if (imgData.sequenceName != "") {
				ss << "_" << imgData.sequenceName.cppStr();
			}
			const bool hasFrameNumber = t.second.size() > 1;
			if (hasFrameNumber) {
				ss << "_" << std::setw(3) << std::setfill('0') << imgData.frameNumber;
			}

			imgData.filenames.emplace_back(ss.str());
			if (frameN == 0) {
				imgData.filenames.emplace_back(":img:" + spriteName);
			}

			i++;
		}
	}

	return frameData;
}
