#include "image_polygon_extractor.h"

#include "halley/graphics/material/material.h"
#include "halley/graphics/sprite/sprite.h"
#include "halley/maths/polygon.h"

using namespace Halley;

Polygon ImagePolygonExtractor::extractPolygon(const Sprite& sprite)
{
	if (!sprite.hasMaterial()) {
		return {};
	}

	const auto tex = sprite.getMaterial().getTexture(0);
	if (!tex) {
		return {};
	}

	const auto img = tex->tryGetOriginalImage();
	if (!img) {
		return {};
	}

	const auto r = sprite.getTexRect() * Vector2f(img->getSize());

	return extractPolygon(*img, Rect4i(r));
}

Polygon ImagePolygonExtractor::extractPolygon(const Image& image, Rect4i bounds)
{
	const Rect4f b = Rect4f(bounds);

	// TODO

	VertexList vs;
	vs += b.getTopCenter();
	vs += b.getCenterRight();
	vs += b.getBottomCenter();
	vs += b.getCenterLeft();
	return Polygon(std::move(vs));
}
