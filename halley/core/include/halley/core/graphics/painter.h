#pragma once
#include "camera.h"
#include "blend.h"
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

		// Draw sprites takes a single vertex per sprite, duplicates the data across multiple vertices, and draws
		// vertPosOffset is the offset, in bytes, from the start of each vertex's data, to a Vector2f which will be filled with the vertex's position in 0-1 space.
		void drawSprites(std::shared_ptr<Material> material, size_t numSprites, size_t vertPosOffset, const void* vertexData);

		// Draws quads to the screen
		void drawQuads(std::shared_ptr<Material> material, size_t numVertices, const void* vertexData);

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
		Vector<char> vertexBuffer;
		std::shared_ptr<Material> materialPending;

		size_t nDrawCalls = 0;
		size_t nVertices = 0;
		size_t nTriangles = 0;

		void checkPendingMaterial(std::shared_ptr<Material>& material);
		void flushPending();
		void executeDrawQuads(Material& material, size_t numVertices, void* vertexData);
		void makeSpaceForPendingBytes(size_t numBytes);
	};
}
