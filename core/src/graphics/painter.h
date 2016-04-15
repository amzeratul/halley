#pragma once
#include "camera.h"
#include "blend.h"
#include <condition_variable>

namespace Halley
{
	class Material;

	class Painter
	{
		friend class RenderContext;

	public:
		virtual ~Painter() {}

		virtual void startRender() = 0;
		virtual void endRender() = 0;

		virtual void clear(Colour colour) = 0;
		virtual void setBlend(BlendType blend) = 0;

		void drawQuads(Material& material, size_t numVertices, void* vertexData);

	protected:
		virtual void setVertices(Material& material, size_t numVertices, void* vertexData) = 0;
		virtual void drawQuads(size_t n) = 0;
		
	private:
		void bind(RenderContext& context);
		RenderContext* activeContext = nullptr;
		Matrix4f projection;
	};
}
