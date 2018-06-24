#include "aseprite_file.h"
#include <halley/support/exception.h>
#include "halley/utils/utils.h"
#include "halley/support/logger.h"
using namespace Halley;

// Specs are at https://github.com/aseprite/aseprite/blob/master/docs/ase-file-specs.md

AsepriteFrame::AsepriteFrame(uint16_t duration)
	: duration(duration)
{}


AsepriteFile::AsepriteFile()
{
}

void AsepriteFile::load(gsl::span<const gsl::byte> data)
{
	Expects(sizeof(AsepriteFileHeader) == 128);

	size_t pos = 0;

	// Read header
	AsepriteFileHeader fileHeader;
	if (data.size() < sizeof(fileHeader)) {
		throw Exception("Invalid Aseprite file (too small)");
	}
	memcpy(&fileHeader, data.data(), sizeof(fileHeader));
	if (fileHeader.magicNumber != 0xA5E0) {
		throw Exception("Invalid Aseprite file (invalid file magic number)");
	}
	pos += sizeof(fileHeader);

	// Store header data
	size = Vector2i(int(fileHeader.width), int(fileHeader.height));
	flags = fileHeader.flags;
	transparentEntry = fileHeader.transparentPaletteEntry;
	numOfColours = fileHeader.numberOfColours;
	switch (fileHeader.colourDepth) {
	case 8:
		colourDepth = AsepriteDepth::Indexed8;
		break;
	case 16:
		colourDepth = AsepriteDepth::Greyscale16;
		break;
	case 32:
		colourDepth = AsepriteDepth::RGBA32;
		break;
	default:
		throw Exception("Unknow colour depth in Aseprite: " + toString(fileHeader.colourDepth));
	}

	// Read each frame
	for (int i = 0; i < int(fileHeader.frames); ++i) {
		const size_t frameStartPos = pos;

		AsepriteFrameHeader frameHeader;
		memcpy(&frameHeader, data.data() + frameStartPos, sizeof(frameHeader));
		if (frameHeader.magicNumber != 0xF1FA) {
			throw Exception("Invalid Aseprite file (invalid frame magic number)");
		}
		addFrame(frameHeader.duration);

		// Read each chunk
		for (int j = 0; j < int(frameHeader.chunks); ++j) {
			const size_t chunkStartPos = pos;

			AsepriteChunkHeader chunkHeader;
			memcpy(&chunkHeader, data.data() + chunkStartPos, sizeof(chunkHeader));
			addChunk(chunkHeader.type, data.subspan(chunkStartPos + sizeof(chunkHeader), chunkHeader.dataSize - sizeof(chunkHeader)));

			pos = chunkStartPos + chunkHeader.dataSize;
		}

		// Next frame
		pos = frameStartPos + frameHeader.dataSize;
	}

	Logger::logInfo("Parsed ase file just fine");
	// Now just do everything else.
}

void AsepriteFile::addFrame(uint16_t duration)
{
	frames.emplace_back(duration);
}

void AsepriteFile::addChunk(uint16_t chunkType, gsl::span<const std::byte> data)
{
	switch (chunkType) {
	case 0x0004:
		// Old palette chunk
		break;
	case 0x0011:
		// Old palette chunk
		break;
	case 0x2004:
		addLayerChunk(data);
		break;
	case 0x2005:
		addCelChunk(data);
		break;
	case 0x2006:
		addCelExtraChunk(data);
		break;
	case 0x2016:
		// Mask chunk (deprecated)
		break;
	case 0x2017:
		// Path chunk (not used)
		break;
	case 0x2018:
		addTagsChunk(data);
		break;
	case 0x2019:
		addPaletteChunk(data);
		break;
	case 0x2020:
		// User data chunk
		break;
	case 0x2022:
		// Slice chunk
		break;
	default:
		// Unknown chunk
		break;
	}
}

void AsepriteFile::addLayerChunk(gsl::span<const std::byte> span)
{
	struct LayerData
	{
		uint16_t flags;
		uint16_t layerType;
		uint16_t childLevel;
		uint16_t defaultWidth;
		uint16_t defaultHeight;
		uint16_t blendMode;
		uint8_t opacity;
		std::array<uint8_t, 3> reserved;
	};

	LayerData data;
	if (span.size() < sizeof(data)) {
		throw Exception("Invalid layer data");
	}
	memcpy(&data, span.data(), sizeof(data));

	layers.push_back(AsepriteLayer());
	auto& layer = layers.back();

	layer.type = AsepriteLayerType(data.layerType);
	layer.childLevel = data.childLevel;
	layer.visible = (data.flags & 1) != 0;
	layer.editable = (data.flags & 2) != 0;
	layer.lockMovement = (data.flags & 4) != 0;
	layer.background = (data.flags & 8) != 0;
	layer.preferLinkedCels = (data.flags & 16) != 0;
	layer.layerGroupDisplaysCollapsed = (data.flags & 32) != 0;
	layer.referenceLayer = (data.flags & 64) != 0;
	layer.opacity = (flags & 1) != 0 ? data.opacity : 255;
	layer.layerName = reinterpret_cast<const char*>(span.data() + sizeof(data));
}

void AsepriteFile::addCelChunk(gsl::span<const std::byte> span)
{
	// TODO
}

void AsepriteFile::addCelExtraChunk(gsl::span<const std::byte> span)
{
	// TODO
}

void AsepriteFile::addPaletteChunk(gsl::span<const std::byte> span)
{
	// TODO
}

void AsepriteFile::addTagsChunk(gsl::span<const std::byte> span)
{
	// TODO
}
