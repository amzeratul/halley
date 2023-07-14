#include "halley/tools/distance_field/distance_field_generator.h"
#include <cassert>
#include <halley/file_formats/image.h>
#include <gsl/gsl_assert>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "../msdfgen/msdfgen.h"
#include "../msdfgen/msdfgen-ext.h"

using namespace Halley;

namespace {
	float getDistanceAt(const int* src, int srcW, int srcH, int xCentre, int yCentre, float radius)
	{
		auto getAlpha = [&](int x, int y) { return (src[x + y * srcW] & 0xFF000000) >> 24; };
		bool isInside = getAlpha(xCentre, yCentre) > 127;
		if (radius < 0.001f) {
			return isInside ? 1.0f : 0.0f;
		}

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
					bestDistSqr = std::min(distSqr, bestDistSqr);
				}
			}
		}

		const float dist = float(sqrt(bestDistSqr));
		const float normalDistance = (2 * dist - 1) / (2 * radius);
		const float finalValue = 0.5f * (isInside ? 1.0f + normalDistance : 1.0f - normalDistance);

		return finalValue;
	}

	std::unique_ptr<Image> generateSDFInternal(Image& srcImg, Vector2i size, float radius)
	{
		Expects(!srcImg.getPixelBytes().empty());
		Expects(srcImg.getFormat() == Image::Format::RGBA);
		const int srcW = srcImg.getWidth();
		const int srcH = srcImg.getHeight();
		const auto src = srcImg.getPixels4BPP();

		auto dstImg = std::make_unique<Image>(Image::Format::SingleChannel, size);

		const int w = size.x;
		const int h = size.y;
		const auto dstStart = dstImg->getPixelBytes();

		int texelW = srcW / w;
		int texelH = srcH / h;

		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				unsigned char* dst = &dstStart[x + y * w];
				float distAcc = 0;
				// For each sub-pixel, compute the distance to closest pixel of the opposite value
				// Then average it all
				for (int j = 0; j < texelH; j++) {
					for (int i = 0; i < texelW; i++) {
						distAcc += getDistanceAt(src.data(), srcW, srcH, x * srcW / w + i, y * srcH / h + j, radius * srcW / w);
					}
				}
				int distance = clamp(int(distAcc * 255 / (texelW * texelH)), 0, 255);
				*dst = static_cast<unsigned char>(distance);
			}
		}

		return dstImg;
	}

	template <int BPP>
	std::unique_ptr<Image> msdfgenImageToHalleyImage(const msdfgen::Bitmap<float, BPP>& src)
	{
		const auto fmt = BPP == 1 ? Image::Format::SingleChannel : Image::Format::RGBA;
		auto result = std::make_unique<Image>(fmt, Vector2i(src.width(), src.height()), false);

		const auto dstPixels = result->getPixels1BPP();
		const auto srcPixels = gsl::span<const float>(src(0, 0), src.width() * src.height() * BPP);
		assert(dstPixels.size() == srcPixels.size());

		const auto n = srcPixels.size();
		for (size_t i = 0; i < n; ++i) {
			dstPixels[i] = static_cast<uint8_t>(clamp(srcPixels[i] * 255.0f, 0.0f, 255.0f));
		}

		return result;
	}
}

std::unique_ptr<Image> DistanceFieldGenerator::generateSDF(Image& srcImg, Vector2i size, float radius)
{
	return generateSDFInternal(srcImg, size, radius);
}

std::unique_ptr<Image> DistanceFieldGenerator::generateMSDF(Image& src, Vector2i size, float radius)
{
	// TODO
	return {};
}

std::unique_ptr<Image> DistanceFieldGenerator::generateSDF2(const FontFace& fontFace, int charcode, Vector2i size, float radius)
{
	const auto font = msdfgen::adoptFreetypeFont(static_cast<FT_Face>(fontFace.getFreeTypeFace()));
	auto bmp = msdfgen::Bitmap<float, 1>(size.x, size.y);

	const double range = radius;
	const double scale = 2.5f;
	const auto pos = Vector2d(Vector2f(radius, radius) / scale);

	msdfgen::Shape shape;
	if (msdfgen::loadGlyph(shape, font, charcode)) {
		shape.inverseYAxis = true;
		shape.normalize();
		const auto bounds = shape.getBounds();

		edgeColoringSimple(shape, 3.0);
		msdfgen::Projection projection({ scale, scale }, { pos.x - bounds.l, pos.y - bounds.b });
		msdfgen::GeneratorConfig config;
		msdfgen::generateSDF(bmp, shape, projection, range, config);
    }

	msdfgen::destroyFont(font);

	return msdfgenImageToHalleyImage(bmp);
}
