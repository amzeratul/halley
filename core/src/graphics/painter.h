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

		void startRender();
		void endRender();
		void flush();

		virtual void clear(Colour colour) = 0;
		virtual void setBlend(BlendType blend) = 0;

		void drawQuads(std::shared_ptr<Material> material, size_t numVertices, void* vertexData);

	protected:
		virtual void doStartRender() = 0;
		virtual void doEndRender() = 0;
		virtual void setVertices(Material& material, size_t numVertices, void* vertexData) = 0;
		virtual void drawQuads(size_t n) = 0;

		virtual void setViewPort(Rect4i rect, bool enableScissor) = 0;
		
	private:
		void bind(RenderContext& context);
		RenderContext* activeContext = nullptr;
		Matrix4f projection;

		size_t verticesPending = 0;
		size_t bytesPending = 0;
		std::vector<char> vertexBuffer;
		std::shared_ptr<Material> materialPending;

		void flushPending();
		void executeDrawQuads(Material& material, size_t numVertices, void* vertexData);
	};
}
