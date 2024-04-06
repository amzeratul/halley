#include "halley/file_formats/image_mask.h"
#include "halley/file_formats/image.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/bytes/compression.h"

using namespace Halley;

ImageMask::ImageMask(Vector2i size, bool clear)
	: size(size)
{
	const size_t numElems = alignUp(size.x * size.y, 8) / 8;
	if (clear) {
		values.resize(numElems, 0);
	} else {
		values.resize(numElems);
	}
}

ImageMask ImageMask::fromAlpha(const Image& image)
{
	const auto size = image.getSize();
	auto result = ImageMask(size, false);

	// This could be faster
	if (image.getFormat() == Image::Format::RGBA || image.getFormat() == Image::Format::RGBAPremultiplied) {
		auto pxs = image.getPixels4BPP();

		for (int y = 0; y < size.y; ++y) {
			for (int x = 0; x < size.x; ++x) {
				const auto px = pxs[x + y * size.x];
				const auto alpha = (px >> 24) & 0xFF;
				result.set(Vector2i(x, y), alpha > 0);
			}
		}
	} else if (image.getFormat() == Image::Format::SingleChannel || image.getFormat() == Image::Format::Indexed) {
		auto pxs = image.getPixels1BPP();

		for (int y = 0; y < size.y; ++y) {
			for (int x = 0; x < size.x; ++x) {
				const auto px = pxs[x + y * size.x];
				// Assume px 0 = transparent
				result.set(Vector2i(x, y), px > 0);
			}
		}
	}

	return result;
}

bool ImageMask::isSet(Vector2i pos) const
{
	const size_t pxIdx = pos.x + pos.y * size.x;
	const size_t byteIdx = pxIdx / 8;
	const int mask = 1 << static_cast<uint8_t>(pxIdx % 8);
	return values[byteIdx] & mask;
}

void ImageMask::set(Vector2i pos, bool value)
{
	const size_t pxIdx = pos.x + pos.y * size.x;
	const size_t byteIdx = pxIdx / 8;
	const int mask = 1 << static_cast<uint8_t>(pxIdx % 8);
	if (value) {
		values[byteIdx] |= mask;
	} else {
		values[byteIdx] &= ~mask;
	}
}

Vector2i ImageMask::getSize() const
{
	return size;
}

Rect4i ImageMask::getRect() const
{
	return Rect4i({}, size);
}

size_t ImageMask::getSizeBytes() const
{
	return sizeof(*this) + values.size();
}

void ImageMask::serialize(Serializer& s) const
{
	s << size;
	s << values;
	//Compression::LZ4Options options;
	//const auto result = Compression::lz4Compress(values.byte_span(), options);
	//s << result;
}

void ImageMask::deserialize(Deserializer& s)
{
	s >> size;
	s >> values;

	//values.resize(alignUp(size.x * size.y, 8) / 8);
	//Bytes compressed;
	//s >> compressed;
	//Compression::lz4Decompress(compressed, values.span());
}
