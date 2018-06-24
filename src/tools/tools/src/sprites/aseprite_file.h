#pragma once

#include <cstdint>
#include <array>
#include <gsl/gsl>
#include "halley/maths/vector2.h"

namespace Halley {
	enum class AsepriteDepth
	{
		RGBA32,
		Greyscale16,
		Indexed8
	};

	struct AsepriteCel
	{
	public:
		Vector2i pos;
		Vector2i size;
		uint16_t layer = 0;
		uint16_t linkedFrame = 0;
		uint8_t opacity = 255;
		bool linked = false;

		Bytes rawData;
	};

	struct AsepriteFrame
	{
	public:
		AsepriteFrame(uint16_t duration);

		uint16_t duration;
		std::vector<AsepriteCel> cels;
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
