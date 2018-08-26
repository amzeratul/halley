#include "aseprite_file.h"
#include <halley/support/exception.h>
#include "halley/utils/utils.h"
#include "halley/support/logger.h"
#include "halley/file/compression.h"
#include <limits>
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
    std::array<uint8_t, 8> _reserved0;
    uint8_t transparentPaletteEntry;
    std::array<uint8_t, 3> _reserved1;
    uint16_t numberOfColours;
    uint8_t pixelWidth;
    uint8_t pixelHeight;
    std::array<uint8_t, 92> _reserved2;
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
	constexpr static size_t size = 16;

	uint16_t flags;
	uint16_t layerType;
	uint16_t childLevel;
	uint16_t defaultWidth;
	uint16_t defaultHeight;
	uint16_t blendMode;
	uint8_t opacity;
	std::array<uint8_t, 3> _reserved;
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
	std::array<uint8_t, 7> _reserved;
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

struct AsepriteFilePaletteData
{
	constexpr static size_t size = 20;

	uint32_t numEntries;
	uint32_t firstIndex;
	uint32_t lastIndex;
	std::array<uint8_t, 8> _reserved;
};

struct AsepriteFilePaletteEntryData
{
	constexpr static size_t size = 6;
	uint16_t flags;
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
};

struct AsepriteFileTagsData
{
	constexpr static size_t size = 10;

	uint16_t numOfTags;
	std::array<uint8_t, 8> _reserved;
};

struct AsepriteFileTagsEntryData
{
	constexpr static size_t size = 17;

	uint16_t fromFrame;
	uint16_t toFrame;
	uint8_t loopDirection;
	std::array<uint8_t, 8> _reserved0;
	std::array<uint8_t, 3> tagColourRGB;
	uint8_t _reserved1;
};


void AsepriteCel::loadImage(AsepriteDepth depth, const std::vector<uint32_t>& palette)
{
	imgData = std::make_unique<Image>(Image::Format::RGBA, size);
	imgData->clear(Image::convertRGBAToInt(0, 0, 0, 0));

	auto dst = reinterpret_cast<uint32_t*>(imgData->getPixels());
	const size_t n = size_t(size.x * size.y);

	if (depth == AsepriteDepth::Indexed8) {
		const auto src = reinterpret_cast<uint8_t*>(rawData.data());
		for (size_t i = 0; i < n; ++i) {
			dst[i] = palette[src[i]];
		}
	} else if (depth == AsepriteDepth::Greyscale16) {
		const auto src = reinterpret_cast<uint16_t*>(rawData.data());
		for (size_t i = 0; i < n; ++i) {
			const auto s = src[i];
			const auto col = uint8_t(s & 0xFF);
			const auto alpha = uint8_t((s >> 8) & 0xFF);
			dst[i] = Image::convertRGBAToInt(col, col, col, alpha);
		}
	} else if (depth == AsepriteDepth::RGBA32) {
		const auto src = reinterpret_cast<uint32_t*>(rawData.data());
		for (size_t i = 0; i < n; ++i) {
			/*
			const auto s = src[i];
			const auto r = s & 0xFF;
			const auto g = (s >> 8) & 0xFF;
			const auto b = (s >> 16) & 0xFF;
			const auto a = (s >> 24) & 0xFF;
			dst[i] = (r << 24) | (g << 16) | (b << 8) | a;
			*/
			dst[i] = src[i];
		}
	}
}

void AsepriteCel::drawAt(Image& dstImage, uint8_t opacity, AsepriteBlendMode blendMode) const
{
	if (!imgData) {
		throw Exception("imgData not loaded.", HalleyExceptions::Tools);
	}

	if (blendMode == AsepriteBlendMode::Normal) {
		dstImage.drawImageAlpha(*imgData, pos, opacity);
	} else if (blendMode == AsepriteBlendMode::Lighten) {
		dstImage.drawImageLighten(*imgData, pos, opacity);
	} else {
		throw Exception("Unsupported blending mode: " + toString(int(blendMode)), HalleyExceptions::Tools);
	}
}

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
		throw Exception("Invalid Aseprite file (too small)", HalleyExceptions::Tools);
	}
	memcpy(&fileHeader, data.data(), AsepriteFileHeader::size);
	if (fileHeader.magicNumber != 0xA5E0) {
		throw Exception("Invalid Aseprite file (invalid file magic number)", HalleyExceptions::Tools);
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
		throw Exception("Unknow colour depth in Aseprite: " + toString(fileHeader.colourDepth), HalleyExceptions::Tools);
	}

	// Read each frame
	for (int i = 0; i < int(fileHeader.frames); ++i) {
		const size_t frameStartPos = pos;

		AsepriteFileFrameHeader frameHeader;
		memcpy(&frameHeader, data.data() + frameStartPos, AsepriteFileFrameHeader::size);
		if (frameHeader.magicNumber != 0xF1FA) {
			throw Exception("Invalid Aseprite file (invalid frame magic number)", HalleyExceptions::Tools);
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
}

void AsepriteFile::addFrame(uint16_t duration)
{
	frames.emplace_back(duration);
}

void AsepriteFile::addChunk(uint16_t chunkType, gsl::span<const gsl::byte> data)
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

void AsepriteFile::addLayerChunk(gsl::span<const gsl::byte> span)
{
	AsepriteFileLayerData data;
	readData(data, span);

	layers.push_back(AsepriteLayer());
	auto& layer = layers.back();

	layer.type = AsepriteLayerType(data.layerType);
	layer.blendMode = AsepriteBlendMode(data.blendMode);
	layer.childLevel = data.childLevel;
	layer.visible = (data.flags & 1) != 0;
	layer.editable = (data.flags & 2) != 0;
	layer.lockMovement = (data.flags & 4) != 0;
	layer.background = (data.flags & 8) != 0;
	layer.preferLinkedCels = (data.flags & 16) != 0;
	layer.layerGroupDisplaysCollapsed = (data.flags & 32) != 0;
	layer.referenceLayer = (data.flags & 64) != 0;
	layer.opacity = (flags & 1) != 0 ? data.opacity : 255;

	layer.layerName = readString(span);
}

void AsepriteFile::addCelChunk(gsl::span<const gsl::byte> span)
{
	AsepriteCel cel;

	AsepriteFileCelData baseData;
	readData(baseData, span);

	cel.opacity = baseData.opacity;
	cel.pos = Vector2i(int(baseData.x), int(baseData.y));
	cel.layer = baseData.layerIndex;

	int type = int(baseData.type_a);
	if (type == 0 || type == 2) {
		// Raw or compressed
		AsepriteFileRawCelData header;
		readData(header, span);

		cel.size = Vector2i(int(header.width), int(header.height));

		// Read pixels
		if (type == 0) {
			// Raw
			cel.rawData.resize(cel.size.x * cel.size.y * getBPP());
			if (span.size() < int(cel.rawData.size())) {
				throw Exception("Invalid cel data", HalleyExceptions::Tools);
			}
			memcpy(cel.rawData.data(), span.data(), cel.rawData.size());
		} else if (type == 2) {
			// ZLIB compressed
			cel.rawData = Compression::decompressRaw(span, std::numeric_limits<size_t>::max());
		}
	} else if (type == 1) {
		// Linked
		AsepriteFileLinkedCelData header;
		readData(header, span);

		cel.linked = true;
		cel.linkedFrame = header.framePosition;
	} else {
		throw Exception("Invalid cel type: " + toString(type), HalleyExceptions::Tools);
	}

	frames.back().cels.push_back(std::move(cel));
}

void AsepriteFile::addCelExtraChunk(gsl::span<const gsl::byte> span)
{
	// TODO
}

void AsepriteFile::addPaletteChunk(gsl::span<const gsl::byte> span)
{
	AsepriteFilePaletteData baseData;
	readData(baseData, span);

	if (paletteBg.empty()) {
		paletteBg.resize(256, 0);
	}

	for (size_t i = baseData.firstIndex; i <= baseData.lastIndex; ++i) {
		AsepriteFilePaletteEntryData entry;
		readData(entry, span);

		paletteBg.at(i) = Image::convertRGBAToInt(entry.red, entry.green, entry.blue, entry.alpha);
		if ((entry.flags & 1) != 0) {
			// Discard name
			readString(span);
		}
	}

	paletteTransparent = paletteBg;
	paletteTransparent[transparentEntry] = 0;
}

void AsepriteFile::addTagsChunk(gsl::span<const gsl::byte> span)
{
	AsepriteFileTagsData baseData;
	readData(baseData, span);

	for (int i = 0; i < baseData.numOfTags; ++i) {
		tags.push_back(AsepriteTag());
		auto& tag = tags.back();

		AsepriteFileTagsEntryData entry;
		readData(entry, span);
		tag.fromFrame = entry.fromFrame;
		tag.toFrame = entry.toFrame;
		tag.animDirection = AsepriteAnimationDirection();
		tag.name = readString(span);
	}
}

AsepriteCel* AsepriteFile::getCelAt(int frameNumber, int layerNumber)
{
	if (frameNumber < 0 || frameNumber >= int(frames.size())) {
		throw Exception("Invalid frame number", HalleyExceptions::Tools);
	}

	auto& frame = frames[frameNumber];
	for (auto& cel: frame.cels) {
		if (cel.layer == layerNumber) {
			if (cel.linked) {
				return getCelAt(cel.linkedFrame, layerNumber);
			} else {
				return &cel;
			}
		}
	}
	return nullptr;
}

size_t AsepriteFile::getBPP() const
{
	switch (colourDepth) {
	case AsepriteDepth::Indexed8:
		return 1;
	case AsepriteDepth::Greyscale16:
		return 2;
	case AsepriteDepth::RGBA32:
		return 4;
	default:
		throw Exception("Unknown depth", HalleyExceptions::Tools);
	}
}

const std::vector<AsepriteTag>& AsepriteFile::getTags() const
{
	return tags;
}

std::unique_ptr<Image> AsepriteFile::makeFrameImage(int frameNumber)
{
	auto frameImage = std::make_unique<Image>(Image::Format::RGBA, size);
	frameImage->clear(Image::convertRGBAToInt(0, 0, 0, 0));

	for (int layerNumber = 0; layerNumber < layers.size(); ++layerNumber) {
		auto& layer = layers[layerNumber];
		if (layer.visible) {
			auto* cel = getCelAt(frameNumber, layerNumber);
			if (cel) {
				const uint8_t opacity = uint8_t(clamp((uint32_t(cel->opacity) * uint32_t(layer.opacity)) / 255, uint32_t(0), uint32_t(255)));
				if (!cel->imgData) {
					cel->loadImage(colourDepth, layer.background ? paletteBg : paletteTransparent);
				}
				cel->drawAt(*frameImage, opacity, layer.blendMode);
			}
		}
	}

	frameImage->preMultiply();
	return std::move(frameImage);
}

const AsepriteFrame& AsepriteFile::getFrame(int frameNumber) const
{
	if (frameNumber < 0 || frameNumber >= int(frames.size())) {
		throw Exception("Invalid frame number", HalleyExceptions::Tools);
	}
	return frames[frameNumber];
}

size_t AsepriteFile::getNumberOfFrames() const
{
	return frames.size();
}
