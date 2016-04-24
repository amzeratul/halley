#include "distance_field_generator.h"

using namespace Halley;

std::unique_ptr<Image> DistanceFieldGenerator::generate(Image& src, Vector2i size, float radius)
{
	size_t srcW = src.getWidth();
	size_t srcH = src.getHeight();

	auto dstImg = std::make_unique<Image>(size.x, size.y);

	size_t w = size.x;
	size_t h = size.y;
	int* dstStart = reinterpret_cast<int*>(dstImg->getPixels());

	for (size_t y = 0; y < h; y++) {
		for (size_t x = 0; x < w; x++) {
			int* dst = dstStart + x + y * w;
			bool srcPx = ((src.getPixel(Vector2i(x * srcW / w, y * srcH / h)) & 0xFF000000) >> 24) != 0;
			*dst = Image::getRGBA(srcPx ? 255 : 0, 0, 0, 255);
		}
	}

	return std::move(dstImg);
}
