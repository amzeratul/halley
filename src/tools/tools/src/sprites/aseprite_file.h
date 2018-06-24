#pragma once

#include <cstdint>
#include <array>
#include <gsl/gsl>
#include "halley/maths/vector2.h"

namespace Halley {
    struct AsepriteFileHeader
    {
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

    struct AsepriteFrameHeader
    {
        uint32_t dataSize;
        uint16_t magicNumber;
        uint16_t chunks;
        uint16_t duration;
        std::array<uint8_t, 6> _reserved;
    };

    struct AsepriteChunkHeader
    {
        uint32_t dataSize;
        uint16_t type;
    };

	enum class AsepriteDepth
	{
		RGBA32,
		Greyscale16,
		Indexed8
	};

	struct AsepriteFrame
	{
	public:
		AsepriteFrame(uint16_t duration);

		uint16_t duration;
	};

	enum class AsepriteLayerType
	{
		Normal,
		Group
	};

	enum class AsepriteBlendMode
	{
		Normal,
		Multiply,
		Screen,
		Overlay,
		Darken,
		Lighten,
		ColorDodge,
		ColorBurn,
		HardLight,
		SoftLight,
		Difference,
		Exclusion,
		Hue,
		Saturation,
		Color,
		Luminosity,
		Addition,
		Subtract,
		Divide
	};

	struct AsepriteLayer
	{
		AsepriteLayerType type = AsepriteLayerType::Normal;
		int childLevel = 0;
		bool visible = true;
		bool editable = true;
		bool lockMovement = false;
		bool background = false;
		bool preferLinkedCels = false;
		bool layerGroupDisplaysCollapsed = false;
		bool referenceLayer = false;
		uint8_t opacity = 255;
		String layerName;
	};
        
    class AsepriteFile {
    public:
		AsepriteFile();

		void load(gsl::span<const gsl::byte> data);

	private:
	    void addFrame(uint16_t duration);
	    void addChunk(uint16_t chunkType, gsl::span<const std::byte> data);

	    void addLayerChunk(gsl::span<const std::byte> span);
		void addCelChunk(gsl::span<const std::byte> span);
		void addCelExtraChunk(gsl::span<const std::byte> span);
		void addPaletteChunk(gsl::span<const std::byte> span);
		void addTagsChunk(gsl::span<const std::byte> span);

		Vector2i size;
		uint32_t flags;
		AsepriteDepth colourDepth;
		uint8_t transparentEntry;
		uint16_t numOfColours;

		std::vector<AsepriteFrame> frames;
		std::vector<AsepriteLayer> layers;
    };
}
