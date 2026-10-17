#pragma once
#include "halley/maths/polygon.h"
#include "halley/maths/rect.h"

namespace Halley {
    class Image;
    class Sprite;

    class ImagePolygonExtractor {
    public:
        static Polygon extractPolygon(const Sprite& sprite);
        static Polygon extractPolygon(const Image& image, Rect4i bounds);

    private:
        static Vector<Vector2i> getBoundaryPixels(const Vector<bool>& mask, Rect4i bounds); /// Only care about the outline pixels
        static Vector<Vector2i> simplifyBoundaryPixelsForConvex(Vector<Vector2i> vs, Rect4i bounds); /// If a pixel is in line with two other on either side, it can be removed
        static VertexList makePixelVertices(gsl::span<const Vector2i> vs, const Vector<bool>& mask, Rect4i bounds); /// Generate the vertices around each pixel
        static VertexList postProcessConvexHull(VertexList vs, Rect4f maxBounds); /// Removes low-relevance vertices

    	static VertexList makeVertices(Rect4i bounds); /// Makes a polygon for the rectangle
    };
}
