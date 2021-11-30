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
#include "../../../../contrib/stb_image/stb_image.h"
#include "../../../../contrib/lodepng/lodepng.h"
#include "halley/support/exception.h"
#include "halley/resources/resource_data.h"
#include "halley/text/string_converter.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/support/logger.h"

using namespace Halley;

Image::Image(Format format, Vector2i size)
	: px(nullptr, [](unsigned char*){})
	, dataLen(0)
	, format(format)
{
	setSize(size);
}

Image::Image(gsl::span<const gsl::byte> bytes, Format targetFormat)
	: px(nullptr, [](unsigned char*) {})
{
	load(bytes, targetFormat);
}

Image::Image(const ResourceDataStatic& data)
	: px(nullptr, [](unsigned char*) {})
{
	load(data.getSpan(), Format::Undefined);
}

Image::Image(const ResourceDataStatic& data, const Metadata& meta)
	: px(nullptr, [](unsigned char*) {})
{
	auto format = fromString<Format>(meta.getString("format", "undefined"));
	load(data.getSpan(), format);
}

Image::Image(Image&& other) = default;

Image::~Image()
{
	px.reset();
}

std::unique_ptr<Image> Image::clone()
{
	auto result = std::make_unique<Image>(format, Vector2i(w, h));
	result->blitFrom(Vector2i(), *this);
	return result;
}

void Image::setSize(Vector2i size)
{
	w = size.x;
	h = size.y;
	dataLen = w * h * getBytesPerPixel();
	dataLen += (16 - (dataLen % 16)) % 16;
	if (w > 0 && h > 0)	{
		px = std::unique_ptr<unsigned char, void(*)(unsigned char*)>(new unsigned char[dataLen], [](unsigned char* data) { delete[] data; });
		if (getBytesPerPixel() == 4) {
			Ensures(size_t(px.get()) % 4 == 0);
		}
		Ensures(px.get() != nullptr);
		memset(px.get(), 0, dataLen);
	} else {
		px = std::unique_ptr<unsigned char, void(*)(unsigned char*)>(nullptr, [](unsigned char*) {});
	}
}

size_t Image::getByteSize() const
{
	return dataLen;
}

unsigned int Image::convertRGBAToInt(unsigned int r, unsigned int g, unsigned int b, unsigned int a)
{
	return (a << 24) | (b << 16) | (g << 8) | r;
}

void Image::convertIntToRGBA(unsigned int col, unsigned int& r, unsigned int& g, unsigned int& b, unsigned int& a)
{
	r = col & 0xFF;
	g = (col >> 8) & 0xFF;
	b = (col >> 16) & 0xFF;
	a = (col >> 24) & 0xFF;
}

Colour4c Image::convertIntToColour(unsigned col)
{
	Colour4c result;
	result.r = col & 0xFF;
	result.g = (col >> 8) & 0xFF;
	result.b = (col >> 16) & 0xFF;
	result.a = (col >> 24) & 0xFF;
	return result;
}

int Image::getBytesPerPixel() const
{
	switch (format) {
	case Format::RGBA:
	case Format::RGBAPremultiplied:
		return 4;
	case Format::RGB:
		return 3;
	case Format::Indexed:
	case Format::SingleChannel:
		return 1;
	default:
		throw Exception("Image format is undefined.", HalleyExceptions::Utils);
	}
}

Image::Format Image::getFormat() const
{
	return format;
}

Rect4i Image::getTrimRect() const
{
	if (getBytesPerPixel() != 4) {
		return Rect4i(0, 0, w, h);
	}

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

Rect4i Image::getRect() const
{
	return Rect4i(Vector2i(), w, h);
}

void Image::clear(int colour)
{
	const int bpp = getBytesPerPixel();
	if (bpp == 4) {
		int* dst = reinterpret_cast<int*>(px.get());
		for (unsigned int y = 0; y < h; y++) {
			for (unsigned int x = 0; x < w; x++) {
				*dst++ = colour;
			}
		}
	} else if (bpp == 1) {
		const char col = static_cast<unsigned char>(colour);
		unsigned char* dst = px.get();
		for (unsigned int y = 0; y < h; y++) {
			for (unsigned int x = 0; x < w; x++) {
				*dst++ = col;
			}
		}
	}
}

void Image::blitFrom(Vector2i pos, gsl::span<const unsigned char> buffer, size_t width, size_t height, size_t pitch, size_t srcBpp)
{
	const size_t xMin = std::max(0, -pos.x);
	const size_t yMin = std::max(0, -pos.y);
	const size_t xMax = std::min(size_t(w) - pos.x, width);
	const size_t yMax = std::min(size_t(h) - pos.y, height);

	if (getBytesPerPixel() == 1) {
		unsigned char* dst = px.get() + pos.x + pos.y * w;
		if (srcBpp == 1) {
			const unsigned char* src = reinterpret_cast<const unsigned char*>(buffer.data());
			for (size_t y = yMin; y < yMax; y++) {
				for (size_t x = xMin; x < xMax; x++) {
					size_t pxPos = (x >> 3) + y * pitch;
					int bit = 1 << (int(7 - x) & 7);
					bool active = (src[pxPos] & bit) != 0;
					dst[x + y * w] = active ? 0xFF : 0;
				}
			}
		} else if (srcBpp == 8) {
			const unsigned char* src = reinterpret_cast<const unsigned char*>(buffer.data());
			for (size_t y = yMin; y < yMax; y++) {
				for (size_t x = xMin; x < xMax; x++) {
					dst[x + y * w] = src[x + y * pitch];
				}
			}
		} else if (srcBpp == 32) {
			throw Exception("Cannot blit from 32-bit to 8-bit.", HalleyExceptions::Utils);
		} else {
			throw Exception("Unknown amount of bits per pixel: " + toString(srcBpp), HalleyExceptions::Utils);
		}
	} else if (getBytesPerPixel() == 4) {
		int* dst = reinterpret_cast<int*>(px.get()) + pos.x + pos.y * w;
		if (srcBpp == 1) {
			const unsigned char* src = reinterpret_cast<const unsigned char*>(buffer.data());
			for (size_t y = yMin; y < yMax; y++) {
				for (size_t x = xMin; x < xMax; x++) {
					size_t pxPos = (x >> 3) + y * pitch;
					int bit = 1 << (int(7 - x) & 7);
					bool active = (src[pxPos] & bit) != 0;
					dst[x + y * w] = convertRGBAToInt(255, 255, 255, active ? 255 : 0);
				}
			}
		} else if (srcBpp == 8) {
			const unsigned char* src = reinterpret_cast<const unsigned char*>(buffer.data());
			for (size_t y = yMin; y < yMax; y++) {
				for (size_t x = xMin; x < xMax; x++) {
					dst[x + y * w] = convertRGBAToInt(255, 255, 255, src[x + y * pitch]);
				}
			}
		} else if (srcBpp == 32) {
			const int* src = reinterpret_cast<const int*>(buffer.data());
			for (size_t y = yMin; y < yMax; y++) {
				for (size_t x = xMin; x < xMax; x++) {
					dst[x + y * w] = src[x + y * pitch];
				}
			}
		} else {
			throw Exception("Unknown amount of bits per pixel: " + toString(srcBpp), HalleyExceptions::Utils);
		}
	}
}

void Image::blitFromRotated(Vector2i pos, gsl::span<const unsigned char> buffer, size_t width, size_t height, size_t pitch, size_t bpp)
{
	Expects(getBytesPerPixel() == 4);

	Rect4i dstRect = Rect4i({}, w, h);
	Rect4i srcRect = Rect4i(pos, int(height), int(width)); // Rotated
	Rect4i intersection = dstRect.intersection(srcRect);

	auto xMin = intersection.getLeft();
	auto yMin = intersection.getTop();
	auto xMax = intersection.getRight();
	auto yMax = intersection.getBottom();
	int* dst = reinterpret_cast<int*>(px.get());

	if (bpp == 32) {
		const int* src = reinterpret_cast<const int*>(buffer.data());
		for (auto y = yMin; y < yMax; y++) {
			for (auto x = xMin; x < xMax; x++) {
				auto srcX = y - yMin;
				auto srcY = height - (x - xMin) - 1;
				dst[x + y * w] = src[srcX + srcY * pitch];
			}
		}
	} else {
		throw Exception("Unknown amount of bits per pixel: " + toString(bpp), HalleyExceptions::Utils);
	}
}

void Image::blitFrom(Vector2i pos, const Image& srcImg, bool rotated)
{
	if (rotated) {
		blitFromRotated(pos, srcImg.getPixelBytes(), srcImg.getWidth(), srcImg.getHeight(), srcImg.getWidth(), getBytesPerPixel() * 8);
	} else {
		blitFrom(pos, srcImg.getPixelBytes(), srcImg.getWidth(), srcImg.getHeight(), srcImg.getWidth(), getBytesPerPixel() * 8);
	}
}

void Image::blitFrom(Vector2i dstPos, const Image& srcImg, Rect4i srcArea, bool rotated)
{
	Vector2i pos = dstPos + Vector2i::max(Vector2i(), -srcArea.getTopLeft()); // If srcArea has padding, its top-left will be negative. Use that as an offset.
	Rect4i src = Rect4i(Vector2i(), srcImg.getSize()).intersection(srcArea);
	size_t stride = srcImg.getWidth();
	size_t offset = src.getTop() * stride + src.getLeft();
	if (rotated) {
		blitFromRotated(pos, srcImg.getPixelBytes().subspan(offset * getBytesPerPixel()), src.getWidth(), src.getHeight(), stride, getBytesPerPixel() * 8);
	} else {
		blitFrom(pos, srcImg.getPixelBytes().subspan(offset * getBytesPerPixel()), src.getWidth(), src.getHeight(), stride, getBytesPerPixel() * 8);
	}
}

void Image::blitDownsampled(Image& srcImg, int scale)
{
	Expects(getBytesPerPixel() == 4);
	Expects(scale >= 1);

	const auto srcSize = srcImg.getSize();
	const auto dstSize = getSize();
	const auto rectW = std::min(dstSize.x, srcSize.x / scale);
	const auto rectH = std::min(dstSize.y, srcSize.y / scale);
	auto dst = getPixels4BPP();
	auto src = srcImg.getPixels4BPP();
	
	for (int y = 0; y < rectH; ++y) {
		auto dstRow = dst.subspan(y * dstSize.x, dstSize.x);
		auto srcRow = src.subspan(y * scale * srcSize.x, srcSize.x);
		for (int x = 0; x < rectW; ++x) {
			dstRow[x] = srcRow[x * scale];
		}
	}
}

inline constexpr static uint32_t alphaBlend(uint32_t src, uint32_t dst, uint32_t opacity)
{
	const uint32_t sr = src & 0xFF;
	const uint32_t sg = (src >> 8) & 0xFF;
	const uint32_t sb = (src >> 16) & 0xFF;
	const uint32_t sa = (src >> 24) & 0xFF;
	const uint32_t srcAlpha = (sa * opacity) / 255;

	if (srcAlpha == 0) {
		return dst;
	} else {
		const uint32_t dr = dst & 0xFF;
		const uint32_t dg = (dst >> 8) & 0xFF;
		const uint32_t db = (dst >> 16) & 0xFF;
		const uint32_t da = (dst >> 24) & 0xFF;

		const uint32_t oneMinusSrcAlpha = 255 - srcAlpha;
		const uint32_t dstAlpha = (oneMinusSrcAlpha * da) / 255;
		const uint32_t totalAlpha = srcAlpha + dstAlpha;

		const uint32_t r = (sr * srcAlpha + dr * dstAlpha) / totalAlpha;
		const uint32_t g = (sg * srcAlpha + dg * dstAlpha) / totalAlpha;
		const uint32_t b = (sb * srcAlpha + db * dstAlpha) / totalAlpha;
		const uint32_t a = srcAlpha + dstAlpha * oneMinusSrcAlpha / 255;

		return r | (g << 8) | (b << 16) | (a << 24);
	}
}

inline constexpr static uint32_t lightenBlend(uint32_t src, uint32_t dst, uint32_t opacity)
{
	const uint32_t sr = src & 0xFF;
	const uint32_t sg = (src >> 8) & 0xFF;
	const uint32_t sb = (src >> 16) & 0xFF;
	const uint32_t sa = (src >> 24) & 0xFF;
	const uint32_t srcAlpha = (sa * opacity) / 255;

	if (srcAlpha == 0) {
		return dst;
	} else {
		const uint32_t dr = dst & 0xFF;
		const uint32_t dg = (dst >> 8) & 0xFF;
		const uint32_t db = (dst >> 16) & 0xFF;
		const uint32_t da = (dst >> 24) & 0xFF;

		const uint32_t oneMinusSrcAlpha = 255 - srcAlpha;
		const uint32_t dstAlpha = (oneMinusSrcAlpha * da) / 255;

		const uint32_t r = std::max(sr * srcAlpha / 255, dr);
		const uint32_t g = std::max(sg * srcAlpha / 255, dg);
		const uint32_t b = std::max(sb * srcAlpha / 255, db);
		const uint32_t a = srcAlpha + dstAlpha * oneMinusSrcAlpha / 255;

		return r | (g << 8) | (b << 16) | (a << 24);
	}
}

inline constexpr static uint32_t addBlend(uint32_t src, uint32_t dst, uint32_t opacity)
{
	const uint32_t sr = src & 0xFF;
	const uint32_t sg = (src >> 8) & 0xFF;
	const uint32_t sb = (src >> 16) & 0xFF;
	const uint32_t sa = (src >> 24) & 0xFF;
	const uint32_t srcAlpha = (sa * opacity) / 255;

	if (srcAlpha == 0) {
		return dst;
	} else {
		const uint32_t dr = dst & 0xFF;
		const uint32_t dg = (dst >> 8) & 0xFF;
		const uint32_t db = (dst >> 16) & 0xFF;
		const uint32_t da = (dst >> 24) & 0xFF;

		const uint32_t dstAlpha = da;

		const uint32_t a = std::max(srcAlpha, dstAlpha);
		const uint32_t r = std::min((sr * srcAlpha + dr * dstAlpha) / a, 255u);
		const uint32_t g = std::min((sg * srcAlpha + dg * dstAlpha) / a, 255u);
		const uint32_t b = std::min((sb * srcAlpha + db * dstAlpha) / a, 255u);

		return r | (g << 8) | (b << 16) | (a << 24);
	}
}
template<typename F>
void blendImages(F f, const Image& src, Image& dst, Vector2i pos, uint8_t opacity)
{
	if (dst.getFormat() != Image::Format::RGBA || src.getFormat() != Image::Format::RGBA) {
		throw Exception("Both images must be RGBA for drawing with alpha", HalleyExceptions::Utils);
	}
	uint32_t opacity32 = opacity;

	const Rect4i srcRect = src.getRect().intersection(dst.getRect() - pos);
	const Rect4i dstRect = dst.getRect().intersection(src.getRect() + pos);

	const size_t rectW = srcRect.getWidth();
	const size_t rectH = srcRect.getHeight();
	if (rectW > 0) {
		for (size_t i = 0; i < rectH; ++i) {
			const auto srcData = src.getPixels4BPP().subspan((i + srcRect.getTop()) * src.getWidth() + srcRect.getLeft());
			const auto dstData = dst.getPixels4BPP().subspan((i + dstRect.getTop()) * dst.getWidth() + dstRect.getLeft());
			for (size_t j = 0; j < rectW; ++j) {
				dstData[j] = f(srcData[j], dstData[j], opacity32);
			}
		}
	}
}

void Image::drawImageAlpha(const Image& src, Vector2i pos, uint8_t opacity)
{
	blendImages(alphaBlend, src, *this, pos, opacity);
}

void Image::drawImageAdd(const Image& src, Vector2i pos, uint8_t opacity)
{
	blendImages(addBlend, src, *this, pos, opacity);
}

void Image::drawImageLighten(const Image& src, Vector2i pos, uint8_t opacity)
{
	blendImages(lightenBlend, src, *this, pos, opacity);
}

std::unique_ptr<Image> Image::loadResource(ResourceLoader& loader)
{
	return std::make_unique<Image>(*loader.getStatic(), loader.getMeta());
}

void Image::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<Image&>(resource));
}

void Image::serialize(Serializer& s) const
{
	s << w;
	s << h;
	s << format;
	s << uint64_t(dataLen);
	s << gsl::as_bytes(gsl::span<unsigned char>(px.get(), dataLen));
}

void Image::deserialize(Deserializer& s)
{
	s >> w;
	s >> h;
	s >> format;

	uint64_t len;
	s >> len;
	dataLen = size_t(len);
	px = std::unique_ptr<unsigned char, void(*)(unsigned char*)>(static_cast<unsigned char*>(malloc(dataLen)), [](unsigned char* data) { free(data); });
	auto span = gsl::as_writable_bytes(gsl::span<unsigned char>(px.get(), dataLen));
	s >> span;
}

void Image::load(gsl::span<const gsl::byte> bytes, Format targetFormat)
{
	if (false && isPNG(bytes)) {
		unsigned char* pixels;
		unsigned int x, y;
		lodepng::State state;
		LodePNGColorType colorFormat;
		switch (targetFormat) {
		case Format::Indexed:
		case Format::SingleChannel:
			colorFormat = LCT_GREY;
			break;
		case Format::RGB:
			colorFormat = LCT_RGB;
			break;
		default:
			colorFormat = LCT_RGBA;
		}
		lodepng_decode_memory(&pixels, &x, &y, reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size(), colorFormat, 8);

		px = std::unique_ptr<unsigned char, void(*)(unsigned char*)>(reinterpret_cast<unsigned char*>(pixels), [](unsigned char* data) { free(data); });
		w = x;
		h = y;
		format = targetFormat != Format::Undefined ? targetFormat : Format::RGBA;
		dataLen = w * h * getBytesPerPixel();
	} else {
		int x, y, nComp;

		int channels = 0;
		switch (targetFormat) {
		case Format::Indexed:
		case Format::SingleChannel:
			channels = 1;
			break;
		case Format::RGB:
			channels = 3;
			break;
		default:
			channels = 4;
		}
		format = targetFormat != Format::Undefined ? targetFormat : Format::RGBA;

		uint8_t *pixels = stbi_load_from_memory(reinterpret_cast<stbi_uc const*>(bytes.data()), static_cast<int>(bytes.size()), &x, &y, &nComp, channels);
		if (!pixels) {
			throw Exception("Unable to load image data: " + String(stbi_failure_reason()), HalleyExceptions::Utils);
		}
		px = std::unique_ptr<unsigned char, void(*)(unsigned char*)>(pixels, [](unsigned char* data) { stbi_image_free(data); });
		w = x;
		h = y;
		dataLen = w * h * getBytesPerPixel();
	}

	if (format == Format::RGBA && targetFormat == Format::Undefined) {
		preMultiply();
	}
}

void Image::preMultiply()
{
	Expects(format == Format::RGBA);

	size_t n = w * h;
	unsigned int* data = reinterpret_cast<unsigned int*>(px.get());
	for (size_t i = 0; i < n; i++) {
		unsigned int cur = data[i];
		unsigned int r, g, b, a;
		convertIntToRGBA(cur, r, g, b, a);
		++a;
		data[i] = ((r * a >> 8) & 0xFF)
			    | ((g * a) & 0xFF00)
				| ((b * a << 8) & 0xFF0000)
				| ((a-1) << 24);
	}

	format = Format::RGBAPremultiplied;
}


gsl::span<unsigned char> Image::getPixelBytes()
{
	return gsl::span<unsigned char>(px.get(), getByteSize());
}

gsl::span<const unsigned char> Image::getPixelBytes() const
{
	return gsl::span<const unsigned char>(px.get(), getByteSize());
}

gsl::span<const unsigned char> Image::getPixelBytesRow(int x0, int x1, int y) const
{
	return getPixelBytes().subspan((x0 + y * w) * size_t(getBytesPerPixel()), (x1 - x0) * size_t(getBytesPerPixel()));
}

gsl::span<int> Image::getPixels4BPP()
{
	Expects(getBytesPerPixel() == 4);
	Expects(dataLen >= size_t(w) * size_t(h) * 4);

	return gsl::span<int>(reinterpret_cast<int*>(px.get()), w * h);
}

gsl::span<const int> Image::getPixels4BPP() const
{
	Expects(getBytesPerPixel() == 4);
	Expects(dataLen >= size_t(w) * size_t(h) * 4);

	return gsl::span<const int>(reinterpret_cast<const int*>(px.get()), w * h);
}

int Image::getPixel4BPP(Vector2i pos) const
{
	if (pos.x < 0 || pos.y < 0 || pos.x >= int(w) || pos.y >= int(h)) {
		return 0;
	}
	return getPixels4BPP()[pos.x + pos.y * w];
}

int Image::getPixelAlpha(Vector2i pos) const
{
	return getPixel4BPP(pos) >> 24;
}

gsl::span<const int> Image::getPixelRow4BPP(int x0, int x1, int y) const
{
	return getPixels4BPP().subspan(x0 + y * w, x1 - x0);
}

Bytes Image::savePNGToBytes(bool allowDepthReduce) const
{
	unsigned char* bytes;
	size_t size;

	LodePNGColorType colFormat;
	switch (format) {
	case Format::RGB:
		colFormat = LCT_RGB;
		break;
	case Format::Indexed:
	case Format::SingleChannel:
		colFormat = LCT_GREY;
		break;
	default:
		colFormat = LCT_RGBA;
	}

	//lodepng_encode_memory(&bytes, &size, reinterpret_cast<unsigned char*>(px.get()), w, h, colFormat, 8);
	LodePNGState state;
	lodepng_state_init(&state);
	state.info_raw.colortype = colFormat;
	state.info_raw.bitdepth = 8;
	state.info_png.color.colortype = colFormat;
	state.info_png.color.bitdepth = 8;
	state.encoder.auto_convert = allowDepthReduce ? 1 : 0;
	lodepng_encode(&bytes, &size, reinterpret_cast<unsigned char*>(px.get()), w, h, &state);
	auto errorCode = state.error;
	lodepng_state_cleanup(&state);

	if (errorCode != 0) {
		throw Exception("Failed to encode PNG", HalleyExceptions::Utils);
	}
	
	Bytes result;
	result.resize(size);
	memcpy(result.data(), bytes, size);
	free(bytes);
	return result;
}

Vector2i Image::getImageSize(gsl::span<const gsl::byte> bytes)
{
	if (isPNG(bytes))	{
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

Image::Format Image::getImageFormat(gsl::span<const gsl::byte> bytes)
{
	unsigned int x, y;
	LodePNGState state;
	lodepng_inspect(&x, &y, &state, reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
	LodePNGColorType colorFormat = state.info_png.color.colortype;
	switch (colorFormat) {
	case LCT_GREY:
		return Format::SingleChannel;
	case LCT_PALETTE:
		return Format::Indexed;
	case LCT_RGB:
		return Format::RGB;
	default:
		return Format::RGBA;
	}
}

bool Image::isPNG(gsl::span<const gsl::byte> bytes)
{
	unsigned char pngHeader[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	return bytes.size() >= 8 && memcmp(bytes.data(), pngHeader, 8) == 0;
}
