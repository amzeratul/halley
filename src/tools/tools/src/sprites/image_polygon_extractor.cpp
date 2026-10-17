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

	return extractPolygon(*img, Rect4i(Vector2i(r.getTopLeft().round()), Vector2i(r.getBottomRight().round())));
}

Polygon ImagePolygonExtractor::extractPolygon(const Image& image, Rect4i bounds)
{
	const auto mask = image.makeAlphaMask(bounds);
	const auto boundPixels = simplifyBoundaryPixelsForConvex(getBoundaryPixels(mask, bounds));
	Logger::logInfo("Got " + toString(boundPixels.size()) + " boundary pixels");

	auto vs = makeVertices(bounds);
	convexClip(vs, boundPixels);
	return Polygon(std::move(vs));
}

VertexList ImagePolygonExtractor::makeVertices(Rect4i bounds)
{
	const Rect4f b = Rect4f(bounds);
	VertexList vs;
	vs += b.getTopLeft();
	vs += b.getTopRight();
	vs += b.getBottomRight();
	vs += b.getBottomLeft();
	return vs;
}

Vector<Vector2i> ImagePolygonExtractor::getBoundaryPixels(Vector<bool> mask, Rect4i bounds)
{
	HalleyAssertDebug((int)mask.size() == bounds.getWidth() * bounds.getHeight());
	const int width = bounds.getWidth();
	const int height = bounds.getHeight();
	const int x0 = bounds.getLeft();
	const int y0 = bounds.getTop();

	Vector<Vector2i> result;
	std::deque<Vector2i> queue;

	Vector<bool> closedList;
	Vector<bool> openList;
	closedList.resize(mask.size(), false);
	openList.resize(mask.size(), false);

	auto getIdx = [&](Vector2i p)
	{
		return (p.x - x0) + (p.y - y0) * width;
	};

	auto addToQueue = [&](Vector2i p)
	{
		if (bounds.contains(p)) {
			const auto idx = getIdx(p);
			if (!closedList[idx] && !openList[idx]) {
				openList[idx] = true;
				queue.push_back(p);
			}
		}
	};

	auto visit = [&](Vector2i p)
	{
		const auto idx = getIdx(p);
		if (!closedList[idx]) {
			closedList[idx] = true;
			if (mask[idx]) {
				result += p;
			} else {
				addToQueue(p + Vector2i(-1, 0));
				addToQueue(p + Vector2i(1, 0));
				addToQueue(p + Vector2i(0, -1));
				addToQueue(p + Vector2i(0, 1));
			}
		}
	};

	// Initialise with edges (do it in a loop around the image)
	for (int x = 0; x < width; ++x) {
		addToQueue(Vector2i(x, 0));
	}
	for (int y = 1; y < height - 1; ++y) {
		addToQueue(Vector2i(0, y));
	}
	for (int x = width; --x >= 0; ) {
		addToQueue(Vector2i(x, height - 1));
	}
	for (int y = height - 1; --y >= 1; ) {
		addToQueue(Vector2i(width - 1, y));
	}

	// Keep visiting remaining
	while (!queue.empty()) {
		auto p = queue.front();
		queue.pop_front();
		visit(p);
	}

	return result;
}

Vector<Vector2i> ImagePolygonExtractor::simplifyBoundaryPixelsForConvex(Vector<Vector2i> vs)
{
	// TODO
	return std::move(vs);
}

void ImagePolygonExtractor::convexClip(VertexList& vs, const Vector<Vector2i>& points)
{
	// TODO
}
