#pragma once
#include "camera.h"
#include "halley/graphics/blend.h"
#include "halley/maths/colour.h"
#include <condition_variable>

namespace Halley
{
	class Material;
	class MaterialDefinition;
	class Camera;
	class RenderContext;

	class Painter
	{
		friend class RenderContext;

	public:
		virtual ~Painter() {}

		void startRender();
		void endRender();
		void flush();

		Rect4i getViewPort() const { return viewPort; }
		Camera& getCurrentCamera() const { return *camera; }

		virtual void clear(Colour colour) = 0;
		virtual void setBlend(BlendType blend) = 0;

		void drawQuads(std::shared_ptr<Material> material, size_t numVertices, void* vertexData);

		size_t getNumDrawCalls() const { return nDrawCalls; }
		size_t getNumVertices() const { return nVertices; }
		size_t getNumTriangles() const { return nTriangles; }

	protected:
		virtual void doStartRender() = 0;
		virtual void doEndRender() = 0;
		virtual void setVertices(MaterialDefinition& material, size_t numVertices, void* vertexData) = 0;
		virtual void drawQuads(size_t n) = 0;

		virtual void setViewPort(Rect4i rect, bool enableScissor) = 0;
		
	private:
		void bind(RenderContext& context);

		RenderContext* activeContext = nullptr;
		Matrix4f projection;
		Rect4i viewPort;
		Camera* camera = nullptr;

		size_t verticesPending = 0;
		size_t bytesPending = 0;
		std::vector<char> vertexBuffer;
		std::shared_ptr<Material> materialPending;

		size_t nDrawCalls;
		size_t nVertices;
		size_t nTriangles;

		void flushPending();
		void executeDrawQuads(Material& material, size_t numVertices, void* vertexData);
	};
}
