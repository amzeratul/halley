#include "aseprite_file.h"
#include <halley/support/exception.h>
#include "halley/utils/utils.h"
#include "halley/support/logger.h"
#include "halley/file/compression.h"
using namespace Halley;

// Specs are at https://github.com/aseprite/aseprite/blob/master/docs/ase-file-specs.md

struct AsepriteFileHeader
{
	constexpr static size_t size = 128;

    uint32_t fileSize;
    uint16_t magicNumber;
    uint16_t frames;
    uint16_t width;
    uint16_t height;
    uint16_t colourDepth;
    uint32_t flags;
    uint16_t speed;
    uint32_t _reserved0;
    uint32_t _reserved1;
    uint8_t transparentPaletteEntry;
    std::array<uint8_t, 3> _reserved2;
    uint16_t numberOfColours;
    uint8_t pixelWidth;
    uint8_t pixelHeight;
    std::array<uint8_t, 88> _reserved3;
};

struct AsepriteFileFrameHeader
{
	constexpr static size_t size = 16;

    uint32_t dataSize;
    uint16_t magicNumber;
    uint16_t chunks;
    uint16_t duration;
    std::array<uint8_t, 6> _reserved;
};

struct AsepriteFileChunkHeader
{
	constexpr static size_t size = 6;

    uint32_t dataSize;
    uint16_t type;
};

struct AsepriteFileLayerData
{
	constexpr static size_t size = 18;

	uint16_t flags;
	uint16_t layerType;
	uint16_t childLevel;
	uint16_t defaultWidth;
	uint16_t defaultHeight;
	uint16_t blendMode;
	uint8_t opacity;
	std::array<uint8_t, 3> reserved;
	uint16_t nameLength;
};

struct AsepriteFileCelData
{
	constexpr static size_t size = 16;

	uint16_t layerIndex;
	int16_t x;
	int16_t y;
	uint8_t opacity;
	uint8_t type_a; // wtf is this misaligned bullshit :(
	uint8_t type_b;
	std::array<uint8_t, 7> reserved;
};

struct AsepriteFileRawCelData
{
	constexpr static size_t size = 4;

	uint16_t width;
	uint16_t height;
};

struct AsepriteFileLinkedCelData
{
	constexpr static size_t size = 2;

	uint16_t framePosition;
};


AsepriteFrame::AsepriteFrame(uint16_t duration)
	: duration(duration)
{}


AsepriteFile::AsepriteFile()
{
}

void AsepriteFile::load(gsl::span<const gsl::byte> data)
{
	size_t pos = 0;

	// Read header
	AsepriteFileHeader fileHeader;
	if (data.size() < AsepriteFileHeader::size) {
		throw Exception("Invalid Aseprite file (too small)");
	}
	memcpy(&fileHeader, data.data(), AsepriteFileHeader::size);
	if (fileHeader.magicNumber != 0xA5E0) {
		throw Exception("Invalid Aseprite file (invalid file magic number)");
	}
	pos += AsepriteFileHeader::size;

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

		AsepriteFileFrameHeader frameHeader;
		memcpy(&frameHeader, data.data() + frameStartPos, AsepriteFileFrameHeader::size);
		if (frameHeader.magicNumber != 0xF1FA) {
			throw Exception("Invalid Aseprite file (invalid frame magic number)");
		}
		addFrame(frameHeader.duration);
		pos += AsepriteFileFrameHeader::size;

		// Read each chunk
		for (int j = 0; j < int(frameHeader.chunks); ++j) {
			const size_t chunkStartPos = pos;

			AsepriteFileChunkHeader chunkHeader;
			memcpy(&chunkHeader, data.data() + chunkStartPos, AsepriteFileChunkHeader::size);
			addChunk(chunkHeader.type, data.subspan(chunkStartPos + AsepriteFileChunkHeader::size, chunkHeader.dataSize - AsepriteFileChunkHeader::size));

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
	AsepriteFileLayerData data;
	if (span.size() < AsepriteFileLayerData::size) {
		throw Exception("Invalid layer data");
	}
	memcpy(&data, span.data(), AsepriteFileLayerData::size);
	span = span.subspan(AsepriteFileLayerData::size);

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

	layer.layerName = String(reinterpret_cast<const char*>(span.data()), data.nameLength);
}

void AsepriteFile::addCelChunk(gsl::span<const std::byte> span)
{
	AsepriteCel cel;

	AsepriteFileCelData baseData;
	if (span.size() < AsepriteFileCelData::size) {
		throw Exception("Invalid cel data");
	}
	memcpy(&baseData, span.data(), AsepriteFileCelData::size);
	span = span.subspan(AsepriteFileCelData::size);

	cel.opacity = baseData.opacity;
	cel.pos = Vector2i(int(baseData.x), int(baseData.y));
	cel.layer = baseData.layerIndex;

	int type = int(baseData.type_a);
	if (type == 0 || type == 2) {
		// Raw or compressed
		AsepriteFileRawCelData header;
		if (span.size() < AsepriteFileRawCelData::size) {
			throw Exception("Invalid cel data");
		}
		memcpy(&header, span.data(), AsepriteFileRawCelData::size);
		span = span.subspan(AsepriteFileRawCelData::size);

		cel.size = Vector2i(int(header.width), int(header.height));

		// Read pixels
		if (type == 0) {
			// Raw
			cel.rawData.resize(cel.size.x * cel.size.y);
			if (span.size() < cel.rawData.size()) {
				throw Exception("Invalid cel data");
			}
			memcpy(cel.rawData.data(), span.data(), cel.rawData.size());
		} else if (type == 2) {
			// ZLIB compressed
			cel.rawData = Compression::inflate(span);
		}
	} else if (type == 1) {
		// Linked
		AsepriteFileLinkedCelData header;
		if (span.size() < AsepriteFileLinkedCelData::size) {
			throw Exception("Invalid cel data");
		}
		memcpy(&header, span.data(), AsepriteFileLinkedCelData::size);

		cel.linked = true;
		cel.linkedFrame = header.framePosition;
	} else {
		throw Exception("Invalid cel type: " + toString(type));
	}

	frames.back().cels.push_back(std::move(cel));
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
