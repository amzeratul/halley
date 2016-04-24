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
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#include "image.h"
#include "stb_image/stb_image.h"
#include "lodepng/lodepng.h"
#include "../support/exception.h"
#include "../resources/resource_data.h"

Halley::Image::Image(unsigned int _w, unsigned int _h)
	: px(nullptr, free)
	, dataLen(0)
	, w(_w)
	, h(_h)
	, nComponents(0)
	, preMultiplied(false)
{
	if (w > 0 && h > 0)	{
		nComponents = 4;
		dataLen = w * h * nComponents;
		px.reset(static_cast<char*>(malloc(dataLen)));
	}
}

Halley::Image::Image(String _filename, const Byte* bytes, size_t nBytes, bool _preMultiply)
	: filename(_filename)
	, px(nullptr, free)
	, preMultiplied(false)
{
	load(filename, bytes, nBytes, _preMultiply);
}

Halley::Image::~Image()
{
}

int Halley::Image::getRGBA(int r, int g, int b, int a)
{
	return (a << 24) | (b << 16) | (g << 8) | r;
}

void Halley::Image::blitFrom(const char* buffer, size_t width, size_t height, size_t pitch)
{
	size_t xMax = std::min(size_t(w), width);
	size_t yMax = std::min(size_t(h), height);

	const int* src = reinterpret_cast<const int*>(buffer);
	int* dst = reinterpret_cast<int*>(px.get());
	for (size_t y = 0; y < yMax; y++) {
		for (size_t x = 0; x < xMax; x++) {
			dst[x + y * w] = src[x + y * pitch];
		}
	}
}

std::unique_ptr<Halley::Image> Halley::Image::loadResource(ResourceLoader& loader)
{
	auto data = loader.getStatic();
	return std::make_unique<Image>(loader.getName(), reinterpret_cast<const Byte*>(data->getData()), data->getSize(), false);
}

void Halley::Image::load(String name, const Byte* bytes, size_t nBytes, bool shouldPreMultiply)
{
	filename = name;

	bool useLodePng = name.endsWith(".png");
	if (useLodePng)	{
		unsigned char* pixels;
		unsigned int x, y;
		lodepng_decode_memory(&pixels, &x, &y, bytes, nBytes, LCT_RGBA, 8);
		px.reset(reinterpret_cast<char*>(pixels));
		w = x;
		h = y;
		nComponents = 4;
		dataLen = w * h * nComponents;
	} else {
		int x, y, nComp;
		nComponents = 4;	// Force 4 bpp
		char *pixels = reinterpret_cast<char*>(stbi_load_from_memory(static_cast<stbi_uc const*>(bytes), static_cast<int>(nBytes), &x, &y, &nComp, nComponents));
		if (!pixels) {
			throw Exception("Unable to load image data.");
		}
		px.reset(static_cast<char*>(pixels));
		w = x;
		h = y;
		dataLen = w * h * nComponents;
	}

	if (nComponents == 4 && shouldPreMultiply) {
		preMultiply();
	}
}

void Halley::Image::preMultiply()
{
	size_t n = w*h;
	unsigned int* data = (unsigned int*) px.get();
	for (size_t i=0; i<n; i++) {
		unsigned int cur = data[i];
		unsigned int r = cur & 0xFF;
		unsigned int g = (cur >> 8) & 0xFF;
		unsigned int b = (cur >> 16) & 0xFF;
		unsigned int a = (cur >> 24) + 1;
		data[i] = ((r * a >> 8) & 0xFF)
			    | ((g * a) & 0xFF00)
				| ((b * a << 8) & 0xFF0000)
				| ((a-1) << 24);
	}
	preMultiplied = true;
}

int Halley::Image::getPixel(Vector2i pos) const
{
	if (pos.x < 0 || pos.y < 0 || pos.x >= int(w) || pos.y >= int(h)) return 0;
	return *(int*)(getPixels() + 4*(pos.x + pos.y*w));
}

int Halley::Image::getPixelAlpha(Vector2i pos) const
{
	unsigned int pixel = static_cast<unsigned int>(getPixel(pos));
	return pixel >> 24;
}

void Halley::Image::savePNG(String file)
{
	if (file == "") file = filename;
	lodepng_encode_file(file.c_str(), reinterpret_cast<unsigned char*>(px.get()), w, h, LCT_RGBA, 8);
}
