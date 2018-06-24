#pragma once
#include <gsl/gsl>
#include <map>
#include "halley/text/halleystring.h"
#include "halley/maths/vector4.h"

namespace Halley
{
	class Path;
	class Image;
	struct ImageData;

	class AsepriteExternalReader
	{
	public:
		static std::vector<ImageData> importAseprite(String baseName, gsl::span<const gsl::byte> fileData, bool trim);

	private:
		static std::vector<ImageData> loadImagesFromPath(Path tmp, bool crop);
		static std::map<int, int> getSpriteDurations(Path jsonPath);
		static void processFrameData(String baseName, std::vector<ImageData>& frameData, std::map<int, int> durations);
	};

	class AsepriteReader
	{
		struct AsepriteFileHeader
		{
			uint32_t fileSize;
			uint16_t magicNumber;
			uint16_t frames;
			uint16_t width;
			uint16_t height;
			uint16_t colorDepth;
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

	public:
		static std::vector<ImageData> importAseprite(String baseName, gsl::span<const gsl::byte> fileData, bool trim);
	};
}
