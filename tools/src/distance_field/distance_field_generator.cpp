#include "distance_field_generator.h"
#include <cassert>

using namespace Halley;

static int getDistanceAt(int* src, int srcW, int srcH, int xCentre, int yCentre, float radius)
{
	auto getAlpha = [&](int x, int y) { return (src[x + y * srcW] & 0xFF000000) >> 24; };
	bool isInside = getAlpha(xCentre, yCentre) > 127;

	int iRadius = int(ceil(radius));
	int x0 = std::max(0, xCentre - iRadius);
	int x1 = std::min(xCentre + iRadius, srcW - 1);
	int y0 = std::max(0, yCentre - iRadius);
	int y1 = std::min(yCentre + iRadius, srcH - 1);

	int bestDistSqr = 2147483647;
	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++) {
			bool thisInside = getAlpha(x, y) > 127;
			if (isInside != thisInside) {
				// Candidate for best neighbour
				int distSqr = (x - xCentre) * (x - xCentre) + (y - yCentre) * (y - yCentre);
				if (distSqr < bestDistSqr) {
					bestDistSqr = distSqr;
				}
			}
		}
	}

	float dist = sqrt(float(bestDistSqr));
	float normalDistance = dist / (2 * radius);
	float finalValue = isInside ? 0.5f + normalDistance : 0.5f - normalDistance;

	return clamp(int(finalValue * 255), 0, 255);
}

std::unique_ptr<Image> DistanceFieldGenerator::generate(Image& srcImg, Vector2i size, float radius)
{
	assert(srcImg.getPixels() != nullptr);
	int srcW = srcImg.getWidth();
	int srcH = srcImg.getHeight();
	int* src = reinterpret_cast<int*>(srcImg.getPixels());

	auto dstImg = std::make_unique<Image>(size.x, size.y);

	int w = size.x;
	int h = size.y;
	int* dstStart = reinterpret_cast<int*>(dstImg->getPixels());

	int offX = srcW / (2 * w);
	int offY = srcH / (2 * h);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			int* dst = dstStart + x + y * w;
			int distance = getDistanceAt(src, srcW, srcH, x * srcW / w + offX, y * srcH / h + offY, radius * srcW / w);
			*dst = Image::getRGBA(255, 255, 255, distance);
		}
	}

	return std::move(dstImg);
}
