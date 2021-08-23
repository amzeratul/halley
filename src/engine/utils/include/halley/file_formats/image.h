/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		                                            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include <gsl/gsl>
#include "halley/maths/vector2.h"
#include "halley/text/halleystring.h"
#include "halley/resources/resource.h"
#include "halley/file/path.h"
#include "halley/maths/rect.h"
#include "halley/maths/colour.h"

namespace Halley {
	class ResourceDataStatic;
	class ResourceLoader;

	class Image final : public Resource {
	public:
		enum class Format {
			Undefined,
			Indexed,
			RGB,
			RGBA,
			RGBAPremultiplied,
			SingleChannel
		};

		Image(Format format = Format::RGBA, Vector2i size = {});
		Image(gsl::span<const gsl::byte> bytes, Format format = Format::Undefined);
		explicit Image(const ResourceDataStatic& data);
		Image(const ResourceDataStatic& data, const Metadata& meta);
		~Image();

		std::unique_ptr<Image> clone();

		void setSize(Vector2i size);

		void load(gsl::span<const gsl::byte> bytes, Format format = Format::Undefined);
		Bytes savePNGToBytes(bool allowDepthReduce = true) const;
		static Vector2i getImageSize(gsl::span<const gsl::byte> bytes);
		static Format getImageFormat(gsl::span<const gsl::byte> bytes);
		static bool isPNG(gsl::span<const gsl::byte> bytes);

		gsl::span<unsigned char> getPixelBytes();
		gsl::span<const unsigned char> getPixelBytes() const;
		gsl::span<const unsigned char> getPixelBytesRow(int x0, int x1, int y) const;
		
		int getPixel4BPP(Vector2i pos) const;
		int getPixelAlpha(Vector2i pos) const;
		gsl::span<int> getPixels4BPP();
		gsl::span<const int> getPixels4BPP() const;
		gsl::span<const int> getPixelRow4BPP(int x0, int x1, int y) const;
		size_t getByteSize() const;

		static unsigned int convertRGBAToInt(unsigned int r, unsigned int g, unsigned int b, unsigned int a=255);
		static void convertIntToRGBA(unsigned int col, unsigned int& r, unsigned int& g, unsigned int& b, unsigned int& a);
		static Colour4c convertIntToColour(unsigned int col);

		unsigned int getWidth() const { return w; }
		unsigned int getHeight() const { return h; }
		Vector2i getSize() const { return Vector2i(int(w), int(h)); }

		int getBytesPerPixel() const;
		Format getFormat() const;

		Rect4i getTrimRect() const;
		Rect4i getRect() const;

		void clear(int colour);
		void blitFrom(Vector2i pos, gsl::span<const unsigned char> buffer, size_t width, size_t height, size_t pitch, size_t srcBpp);
		void blitFromRotated(Vector2i pos, gsl::span<const unsigned char> buffer, size_t width, size_t height, size_t pitch, size_t bpp);
		void blitFrom(Vector2i pos, const Image& srcImg, bool rotated = false);
		void blitFrom(Vector2i pos, const Image& srcImg, Rect4i srcArea, bool rotated = false);
		void blitDownsampled(Image& src, int scale);

		void drawImageAlpha(const Image& src, Vector2i pos, uint8_t opacity = 255);
		void drawImageAdd(const Image& src, Vector2i pos, uint8_t opacity = 255);
		void drawImageLighten(const Image& src, Vector2i pos, uint8_t opacity = 255);

		static std::unique_ptr<Image> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Image; }
		void reload(Resource&& resource) override;

		Image& operator=(const Image& o) = delete;
		Image& operator=(Image&& o) = default;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void preMultiply();

	private:
		std::unique_ptr<unsigned char, void(*)(unsigned char*)> px;
		size_t dataLen = 0;
		unsigned int w = 0;
		unsigned int h = 0;
		Format format = Format::Undefined;
	};

	template <>
	struct EnumNames<Image::Format> {
		constexpr std::array<const char*, 6> operator()() const {
			return{{
				"undefined",
				"indexed",
				"rgb",
				"rgba",
				"rgba_premultiplied",
				"single_channel"
			}};
		}
	};
}
