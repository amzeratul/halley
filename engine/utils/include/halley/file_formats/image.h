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

namespace Halley {
	class ResourceDataStatic;
	class ResourceLoader;

	class Image : public Resource {
	public:
		enum class Mode {
			Undefined,
			Indexed,
			RGB,
			RGBA,
			RGBAPremultiplied
		};

		Image(Mode mode = Mode::RGBA, unsigned int w = 0, unsigned int h = 0);
		Image(gsl::span<const gsl::byte> bytes, Mode mode = Mode::Undefined);
		explicit Image(const ResourceDataStatic& data);
		Image(const ResourceDataStatic& data, const Metadata& meta);
		~Image();

		void setSize(Vector2i size);

		void load(gsl::span<const gsl::byte> bytes, Mode mode = Mode::Undefined);
		void savePNG(const String& filename = "") const;
		void savePNG(const Path& filename) const;
		Bytes savePNGToBytes();
		static Vector2i getImageSize(gsl::span<const gsl::byte> bytes);
		static Mode getImageMode(gsl::span<const gsl::byte> bytes);
		static bool isPNG(gsl::span<const gsl::byte> bytes);

		int getPixel(Vector2i pos) const;
		int getPixelAlpha(Vector2i pos) const;
		static int getRGBA(int r, int g, int b, int a=255);
		char* getPixels() { return px.get(); }
		const char* getPixels() const { return px.get(); }
		size_t getByteSize() const;

		unsigned int getWidth() const { return w; }
		unsigned int getHeight() const { return h; }
		Vector2i getSize() const { return Vector2i(int(w), int(h)); }

		int getBytesPerPixel() const;
		Mode getMode() const;

		Rect4i getTrimRect() const;

		void clear(int colour);
		void blitFrom(Vector2i pos, const char* buffer, size_t width, size_t height, size_t pitch, size_t bpp);
		void blitFromRotated(Vector2i pos, const char* buffer, size_t width, size_t height, size_t pitch, size_t bpp);
		void blitFrom(Vector2i pos, Image& img, bool rotated = false);
		void blitFrom(Vector2i pos, Image& img, Rect4i srcArea, bool rotated = false);

		static std::unique_ptr<Image> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Image; }

		Image& operator=(const Image& o) = delete;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		std::unique_ptr<char, void(*)(char*)> px;
		size_t dataLen = 0;
		unsigned int w = 0;
		unsigned int h = 0;
		Mode mode = Mode::Undefined;
		
		void preMultiply();
	};

	template <>
	struct EnumNames<Image::Mode> {
		constexpr std::array<const char*, 5> operator()() const {
			return{{
				"undefined",
				"indexed",
				"rgb",
				"rgba",
				"rgba_premultiplied"
			}};
		}
	};
}
