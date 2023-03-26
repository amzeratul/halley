#pragma once

#include "halley/data_structures/vector.h"
#include "halley/file_formats/image.h"
#include <gsl/span>

namespace Halley {
    class Image;

    class HLIFFile {
    public:
    	struct Info {
            Vector2i size;
        	Image::Format format;
        };

        static void decode(Image& dst, gsl::span<const gsl::byte> data);
        static Bytes encode(const Image& image, std::string_view name = {});
        static Info getInfo(gsl::span<const gsl::byte> data);
        static bool isHLIF(gsl::span<const gsl::byte> data);

    private:
     	constexpr static uint8_t hlifId[8] = "HLIFv01";

    	enum class Format : uint8_t {
			RGBA,
			SingleChannel,
			Indexed
		};

        enum class Flags {
	        Premultiplied = 1,
            HasPalette = 2,
        };

		struct Header {
			uint8_t id[8];
			uint16_t width = 0;
			uint16_t height = 0;
			uint32_t compressedSize = 0;
			uint32_t uncompressedSize = 0;
			Format format = Format::RGBA;
            uint8_t flags = 0;
		};

        // Same as PNG
        enum class LineEncoding: uint8_t {
            None = 0,
            Sub = 1,
            Up = 2,
            Average = 3,
            Paeth = 4
        };
        
    	static void decodeLines(Vector2i size, gsl::span<const uint8_t> lineData, gsl::span<uint8_t> pixelData, int bpp);
    	static void encodeLines(Vector2i size, gsl::span<uint8_t> lineData, gsl::span<uint8_t> pixelData, int bpp);
        static LineEncoding findBestLineEncoding(gsl::span<const uint8_t> curLine, gsl::span<const uint8_t> prevLine, int bpp);
        static void encodeLine(LineEncoding lineEncoding, gsl::span<uint8_t> curLine, gsl::span<const uint8_t> prevLine, int bpp);
        static void decodeLine(LineEncoding lineEncoding, gsl::span<uint8_t> curLine, gsl::span<const uint8_t> prevLine, int bpp);
        static int getBPP(Format format);

        static std::optional<std::pair<Vector<int>, Bytes>> makePalette(gsl::span<const int> pixels, std::string_view name = {});
        static void decodePalette(gsl::span<const uint8_t> palettedImage, gsl::span<const int> palette, gsl::span<int> dst);
    };
}
