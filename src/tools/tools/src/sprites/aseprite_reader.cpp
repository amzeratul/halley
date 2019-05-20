#include "aseprite_reader.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/file/path.h"
#include "halley/file_formats/image.h"
#include "../assets/importers/sprite_importer.h"
#include "aseprite_file.h"
using namespace Halley;


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
	bool firstImage = true;
	for (auto& t: tags) {
		int i = 0;
		String sequence = t.first;
		String direction = "";
		if (sequence.find(':') != std::string::npos) {
			auto split = sequence.split(':');
			sequence = split.at(0);
			direction = split.at(1);
		}
	
		for (const int frameN: t.second) {
			frameData.emplace_back();
			auto& imgData = frameData.back();

			imgData.img = aseFile.makeFrameImage(frameN);
			imgData.frameNumber = i;
			imgData.sequenceName = sequence;
			imgData.direction = direction;
			imgData.duration = aseFile.getFrame(frameN).duration;
			imgData.clip = trim ? imgData.img->getTrimRect() : imgData.img->getRect();

			std::stringstream ss;
			ss << baseName.cppStr();
			if (imgData.sequenceName != "") {
				ss << "_" << imgData.sequenceName.cppStr();
			}
			if (imgData.direction != "") {
				ss << "_" << imgData.direction.cppStr();
			}
			const bool hasFrameNumber = t.second.size() > 1;
			if (hasFrameNumber) {
				ss << "_" << std::setw(3) << std::setfill('0') << imgData.frameNumber;
			}

			imgData.filenames.emplace_back(ss.str());
			if (firstImage) {
				firstImage = false;
				imgData.filenames.emplace_back(":img:" + spriteName);
			}

			i++;
		}
	}

	return frameData;
}
