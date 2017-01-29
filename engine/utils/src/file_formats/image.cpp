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

#include <cassert>
#include "halley/file_formats/image.h"
#include "../../contrib/stb_image/stb_image.h"
#include "../../contrib/lodepng/lodepng.h"
#include "halley/support/exception.h"
#include "halley/resources/resource_data.h"
#include "halley/text/string_converter.h"

Halley::Image::Image(unsigned int _w, unsigned int _h)
	: px(nullptr, [](char*){})
	, dataLen(0)
	, nComponents(0)
{
	setSize(Vector2i(_w, _h));
}

Halley::Image::Image(String _filename, gsl::span<const gsl::byte> bytes, bool _preMultiply)
	: filename(_filename)
	, px(nullptr, [](char*) {})
	, preMultiplied(false)
{
	load(filename, bytes, _preMultiply);
}

Halley::Image::~Image()
{
	px.reset();
}

void Halley::Image::setSize(Vector2i size)
{
	preMultiplied = false;
	w = size.x;
	h = size.y;
	if (w > 0 && h > 0)	{
		nComponents = 4;
		dataLen = w * h * nComponents;
		dataLen += (16 - (dataLen % 16)) % 16;
		px = std::unique_ptr<char, void(*)(char*)>(new char[dataLen], [](char* data) { delete[] data; });
		assert(px.get() != nullptr);
	}
}

void Halley::Image::setName(const String& name)
{
	filename = name;
}

int Halley::Image::getRGBA(int r, int g, int b, int a)
{
	return (a << 24) | (b << 16) | (g << 8) | r;
}

Halley::Rect4i Halley::Image::getTrimRect() const
{
	int x0 = w;
	int x1 = 0;
	int y0 = h;
	int y1 = 0;

	const unsigned int* src = reinterpret_cast<const unsigned int*>(px.get());
	for (int y = 0; y < int(h); y++) {
		for (int x = 0; x < int(w); x++) {
			unsigned int px = src[x + y * w];
			int alpha = int(px >> 24);

			if (alpha > 0) {
				x0 = std::min(x0, x);
				y0 = std::min(y0, y);
				x1 = std::max(x1, x);
				y1 = std::max(y1, y);
			}
		}
	}

	if (x0 > x1 || y0 > y1) {
		return Rect4i();
	}

	return Rect4i(Vector2i(x0, y0), Vector2i(x1 + 1, y1 + 1));
}

void Halley::Image::clear(int colour)
{
	int* dst = reinterpret_cast<int*>(px.get());
	for (unsigned int y = 0; y < h; y++) {
		for (unsigned int x = 0; x < w; x++) {
			*dst++ = colour;
		}
	}
}

void Halley::Image::blitFrom(Vector2i pos, const char* buffer, size_t width, size_t height, size_t pitch, size_t bpp)
{
	size_t xMin = std::max(0, -pos.x);
	size_t yMin = std::max(0, -pos.y);
	size_t xMax = std::min(size_t(w) - pos.x, width);
	size_t yMax = std::min(size_t(h) - pos.y, height);
	int* dst = reinterpret_cast<int*>(px.get()) + pos.x + pos.y * w;

	if (bpp == 1) {
		const char* src = reinterpret_cast<const char*>(buffer);
		for (size_t y = yMin; y < yMax; y++) {
			for (size_t x = xMin; x < xMax; x++) {
				size_t pxPos = (x >> 3) + y * pitch;
				int bit = 1 << (int(7 - x) & 7);
				bool active = (src[pxPos] & bit) != 0;
				dst[x + y * w] = getRGBA(255, 255, 255, active ? 255 : 0);
			}
		}
	} else if (bpp == 8) {
		const char* src = reinterpret_cast<const char*>(buffer);
		for (size_t y = yMin; y < yMax; y++) {
			for (size_t x = xMin; x < xMax; x++) {
				dst[x + y * w] = getRGBA(255, 255, 255, src[x + y * pitch]);
			}
		}
	} else if (bpp == 32) {
		const int* src = reinterpret_cast<const int*>(buffer);
		for (size_t y = yMin; y < yMax; y++) {
			for (size_t x = xMin; x < xMax; x++) {
				dst[x + y * w] = src[x + y * pitch];
			}
		}
	} else {
		throw Exception("Unknown amount of bits per pixel: " + toString(bpp));
	}
}

void Halley::Image::blitFromRotated(Vector2i pos, const char* buffer, size_t width, size_t height, size_t pitch, size_t bpp)
{
	Rect4i dstRect = Rect4i({}, w, h);
	Rect4i srcRect = Rect4i(pos, int(height), int(width)); // Rotated
	Rect4i intersection = dstRect.intersection(srcRect);

	auto xMin = intersection.getLeft();
	auto yMin = intersection.getTop();
	auto xMax = intersection.getRight();
	auto yMax = intersection.getBottom();
	int* dst = reinterpret_cast<int*>(px.get());

	if (bpp == 32) {
		const int* src = reinterpret_cast<const int*>(buffer);
		for (auto y = yMin; y < yMax; y++) {
			for (auto x = xMin; x < xMax; x++) {
				auto srcX = y - yMin;
				auto srcY = height - (x - xMin) - 1;
				dst[x + y * w] = src[srcX + srcY * pitch];
			}
		}
	} else {
		throw Exception("Unknown amount of bits per pixel: " + toString(bpp));
	}
}

void Halley::Image::blitFrom(Vector2i pos, Image& srcImg, bool rotated)
{
	if (rotated) {
		blitFromRotated(pos, srcImg.getPixels(), srcImg.getWidth(), srcImg.getHeight(), srcImg.getWidth(), 32);
	} else {
		blitFrom(pos, srcImg.getPixels(), srcImg.getWidth(), srcImg.getHeight(), srcImg.getWidth(), 32);
	}
}

void Halley::Image::blitFrom(Vector2i pos, Image& srcImg, Rect4i srcArea, bool rotated)
{
	Rect4i src = Rect4i(Vector2i(), srcImg.getSize()).intersection(srcArea);
	size_t stride = srcImg.getWidth();
	size_t offset = src.getTop() * stride + src.getLeft();
	if (rotated) {
		blitFromRotated(pos, srcImg.getPixels() + offset * 4, src.getWidth(), src.getHeight(), stride, 32);
	} else {
		blitFrom(pos, srcImg.getPixels() + offset * 4, src.getWidth(), src.getHeight(), stride, 32);
	}
}

std::unique_ptr<Halley::Image> Halley::Image::loadResource(ResourceLoader& loader)
{
	auto data = loader.getStatic();
	return std::make_unique<Image>(loader.getName(), data->getSpan(), false);
}

void Halley::Image::load(String name, gsl::span<const gsl::byte> bytes, bool shouldPreMultiply)
{
	filename = name;

	bool useLodePng = name.endsWith(".png");
	if (useLodePng)	{
		unsigned char* pixels;
		unsigned int x, y;
		lodepng_decode_memory(&pixels, &x, &y, reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size(), LCT_RGBA, 8);
		px = std::unique_ptr<char, void(*)(char*)>(reinterpret_cast<char*>(pixels), [](char* data) { ::free(data); });
		w = x;
		h = y;
		nComponents = 4;
		dataLen = w * h * nComponents;
	} else {
		int x, y, nComp;
		nComponents = 4;	// Force 4 bpp
		char *pixels = reinterpret_cast<char*>(stbi_load_from_memory(reinterpret_cast<stbi_uc const*>(bytes.data()), static_cast<int>(bytes.size()), &x, &y, &nComp, nComponents));
		if (!pixels) {
			throw Exception("Unable to load image data.");
		}
		px = std::unique_ptr<char, void(*)(char*)>(pixels, [](char* data) { stbi_image_free(data); });
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
	unsigned int* data = reinterpret_cast<unsigned int*>(px.get());
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
	return *reinterpret_cast<const int*>(getPixels() + 4*(pos.x + pos.y*w));
}

int Halley::Image::getPixelAlpha(Vector2i pos) const
{
	unsigned int pixel = static_cast<unsigned int>(getPixel(pos));
	return pixel >> 24;
}

void Halley::Image::savePNG(String file) const
{
	if (file == "") file = filename;
	savePNG(Path(file.cppStr()));
}

void Halley::Image::savePNG(const Path& file) const
{
	lodepng_encode_file(file.string().c_str(), reinterpret_cast<unsigned char*>(px.get()), w, h, LCT_RGBA, 8);
}

Halley::Bytes Halley::Image::savePNGToBytes()
{
	unsigned char* bytes;
	size_t size;
	lodepng_encode_memory(&bytes, &size, reinterpret_cast<unsigned char*>(px.get()), w, h, LCT_RGBA, 8);
	Bytes result;
	result.resize(size);
	memcpy(result.data(), bytes, size);
	free(bytes);
	return result;
}

Halley::Vector2i Halley::Image::getImageSize(String name, gsl::span<const gsl::byte> bytes)
{
	bool useLodePng = name.endsWith(".png");
	if (useLodePng)	{
		unsigned w, h;
		lodepng::State state;
		lodepng_inspect(&w, &h, &state, reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
		return Vector2i(int(w), int(h));
	} else {
		int w, h, comp;
		stbi_info_from_memory(reinterpret_cast<const unsigned char*>(bytes.data()), int(bytes.size()), &w, &h, &comp);
		return Vector2i(w, h);
	}
}
