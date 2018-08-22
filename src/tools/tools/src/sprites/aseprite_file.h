#pragma once

#include <cstdint>
#include <array>
#include <gsl/gsl>
#include "halley/maths/vector2.h"
#include "halley/maths/colour.h"
#include "halley/file_formats/image.h"

namespace Halley {
	enum class AsepriteBlendMode;

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
		std::unique_ptr<Image> imgData;

		void loadImage(AsepriteDepth depth, const std::vector<uint32_t>& palette);
		void drawAt(Image& image, uint8_t opacity, AsepriteBlendMode blendMode) const;
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
		AsepriteBlendMode blendMode = AsepriteBlendMode::Normal;
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

	enum class AsepriteAnimationDirection
	{
		Forward,
		Reverse,
		PingPong
	};

	struct AsepriteTag
	{
		int fromFrame = -1;
		int toFrame = -1;
		AsepriteAnimationDirection animDirection = AsepriteAnimationDirection::Forward;
		
		String name;
	};
        
    class AsepriteFile {
    public:
		AsepriteFile();

		void load(gsl::span<const gsl::byte> data);

		const std::vector<AsepriteTag>& getTags() const;
		std::unique_ptr<Image> makeFrameImage(int n);
	    const AsepriteFrame& getFrame(int n) const;
	    size_t getNumberOfFrames() const;

    private:
	    void addFrame(uint16_t duration);
	    void addChunk(uint16_t chunkType, gsl::span<const gsl::byte> data);

	    void addLayerChunk(gsl::span<const gsl::byte> span);
		void addCelChunk(gsl::span<const gsl::byte> span);
		void addCelExtraChunk(gsl::span<const gsl::byte> span);
		void addPaletteChunk(gsl::span<const gsl::byte> span);
		void addTagsChunk(gsl::span<const gsl::byte> span);

		template <typename T>
		void readData(T& dst, gsl::span<const gsl::byte>& data) const
		{
			if (data.size() < T::size) {
				throw Exception("Insufficient data to decode Aseprite entry", HalleyExceptions::Tools);
			}
			memcpy(&dst, data.data(), T::size);
			data = data.subspan(T::size);
		}

		String readString(gsl::span<const gsl::byte>& data) const
		{
			if (data.size() < 2) {
				throw Exception("Insufficient data to decode Aseprite entry", HalleyExceptions::Tools);
			}
			uint16_t size;
			memcpy(&size, data.data(), 2);
			data = data.subspan(2);
			if (data.size() < size) {
				throw Exception("Insufficient data to decode Aseprite entry", HalleyExceptions::Tools);
			}
			String result = String(reinterpret_cast<const char*>(data.data()), size);
			data = data.subspan(size);
			return result;
		}

	    AsepriteCel* getCelAt(int frameNumber, int layerNumber);
	    size_t getBPP() const;

		Vector2i size;
		uint32_t flags;
		AsepriteDepth colourDepth;
		uint8_t transparentEntry;
		uint16_t numOfColours;

		std::vector<AsepriteFrame> frames;
		std::vector<AsepriteLayer> layers;
		std::vector<AsepriteTag> tags;
		std::vector<uint32_t> paletteBg;
		std::vector<uint32_t> paletteTransparent;
    };
}
