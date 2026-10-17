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
	bounds = image.getRect().intersection(bounds);
	if (bounds.getWidth() == 0 || bounds.getHeight() == 0) {
		//Logger::logError("Bounds " + toString(bounds) + " do not overlap image bounds " + image.getRect());
		return {};
	}

	const auto mask = image.makeAlphaMask(bounds);
	const auto boundPixels = simplifyBoundaryPixelsForConvex(getBoundaryPixels(mask, bounds), bounds);
	auto vertices = makePixelVertices(boundPixels, mask, bounds);
	return postProcessConvexHull(Polygon::makeConvexHull(std::move(vertices)), bounds);
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

Vector<Vector2i> ImagePolygonExtractor::getBoundaryPixels(const Vector<bool>& mask, Rect4i bounds)
{
	HalleyAssertDebug((int)mask.size() == bounds.getWidth() * bounds.getHeight());
	const int width = bounds.getWidth();
	const int height = bounds.getHeight();
	const auto origin = bounds.getTopLeft();
	const auto localBounds = Rect4i(0, 0, width, height);

	Vector<Vector2i> result;
	std::deque<Vector2i> queue;

	Vector<bool> closedList;
	Vector<bool> openList;
	closedList.resize(mask.size(), false);
	openList.resize(mask.size(), false);

	auto getIdx = [&](Vector2i p)
	{
		return p.x + p.y * width;
	};

	auto addToQueue = [&](Vector2i p)
	{
		if (localBounds.contains(p)) {
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
				result += p + origin;
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

Vector<Vector2i> ImagePolygonExtractor::simplifyBoundaryPixelsForConvex(Vector<Vector2i> vs, Rect4i bounds)
{
	const int width = bounds.getWidth();
	const int height = bounds.getHeight();
	const int x0 = bounds.getLeft();
	const int y0 = bounds.getTop();
	const auto offsetBounds = Rect4i(0, 0, width, height);

	auto getOrigIdx = [&](Vector2i p)
	{
		return (p.x - x0) + (p.y - y0) * width;
	};

	// Re-create an image mask for the vertices
	Vector<bool> mask;
	mask.resize(width * height, false);
	for (auto& v: vs) {
		mask[getOrigIdx(v)] = true;
	}

	
	auto getIdx = [&](Vector2i p)
	{
		return p.x + p.y * width;
	};

	auto check = [&] (Vector2i p, Vector2i offset) -> bool
	{
		const auto p1 = p + offset;
		const auto p2 = p - offset;
		if (!offsetBounds.contains(p1) || !offsetBounds.contains(p2)) {
			return false;
		}
		// If both neighbours are set, this point is aligned with them and is therefore not necessary
		return mask[getIdx(p1)] && mask[getIdx(p2)];
	};

	const auto dirs = std::to_array({
		Vector2i(1, 0), // Horizontal
		Vector2i(0, 1), // Vertical
		Vector2i(1, 1), // Diag 1
		Vector2i(1, -1), // Diag 2
		Vector2i(2, 1), // Iso 1
		Vector2i(2, -1), // Iso 2
		Vector2i(1, 2), // Iso 3
		Vector2i(1, -2), // Iso 4
	});

	// Copy only pixels that aren't redundant
	Vector<Vector2i> result;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			const auto p = Vector2i(x, y);
			if (mask[getIdx(p)]) {
				bool valid = true;
				for (const auto& d: dirs) {
					if (check(p, d)) {
						valid = false;
						break;
					}
				}
				if (valid) {
					result += p + Vector2i(x0, y0);
				}
			}
		}
	}
	return result;
}

VertexList ImagePolygonExtractor::makePixelVertices(gsl::span<const Vector2i> vs, const Vector<bool>& mask, Rect4i bounds)
{
	const auto width = bounds.getWidth();

	auto hasNeighbour = [&] (Vector2i p, Vector2i offset) -> bool
	{
		const auto n = p + offset;
		if (bounds.contains(n)) {
			return mask[n.x + n.y * width];
		}
		return false;
	};

	VertexList result;
	result.reserve(vs.size());

	for (const auto& v: vs) {
		const bool left = hasNeighbour(v, Vector2i(-1, 0));
		const bool right = hasNeighbour(v, Vector2i(1, 0));
		const bool up = hasNeighbour(v, Vector2i(0, -1));
		const bool down = hasNeighbour(v, Vector2i(0, 1));
		
		if (left && right && up && down) {
			Logger::logError("Invalid point at " + toString(v));
		}

		if (!left && !up) {
			result += Vector2f(v);
		}
		if (!left && !down) {
			result += Vector2f(v + Vector2i(0, 1));
		}
		if (!right && !up) {
			result += Vector2f(v + Vector2i(1, 0));
		}
		if (!right && !down) {
			result += Vector2f(v + Vector2i(1, 1));
		}

		if (!left && up && down) {
			result += Vector2f(v) + Vector2f(0, 0.5f);
		}
		if (!right && up && down) {
			result += Vector2f(v) + Vector2f(1, 0.5f);
		}
		if (!up && left && right) {
			result += Vector2f(v) + Vector2f(0.5f, 0);
		}
		if (!down && left && right) {
			result += Vector2f(v) + Vector2f(0.5f, 1);
		}
	}

	return result;
}

VertexList ImagePolygonExtractor::postProcessConvexHull(VertexList vs, Rect4i boundsInteger)
{
	const auto bounds = Rect4f(boundsInteger);

	// TODO
	return vs;
}
