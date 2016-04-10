#pragma once

namespace Halley
{
	class Material;

	class Painter
	{
	public:
		virtual ~Painter() {}

		virtual void startRender() = 0;
		virtual void endRender() = 0;

		virtual void drawVertices(Material& material, size_t numVertices, size_t vertexStride, void* vertexData) = 0;
		virtual void clear(Colour colour) = 0;
	};
}
