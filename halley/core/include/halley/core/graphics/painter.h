#pragma once
#include "camera.h"
#include "blend.h"
#include "halley/maths/colour.h"
#include <condition_variable>
#include <halley/maths/vector4.h>

namespace Halley
{
	class Material;
	class MaterialDefinition;
	class Camera;
	class RenderContext;

	class Painter
	{
		friend class RenderContext;

		struct PainterVertexData
		{
			char* dstVertex;
			unsigned short* dstIndex;
			size_t vertexSize;
			size_t dataSize;
			unsigned short firstIndex;
		};

	public:
		virtual ~Painter() {}

		void startRender();
		void endRender();
		void flush();

		Rect4i getViewPort() const { return viewPort; }
		Camera& getCurrentCamera() const { return *camera; }

		virtual void clear(Colour colour) = 0;
		virtual void setBlend(BlendType blend) = 0;

		// Draws quads to the screen
		void drawQuads(std::shared_ptr<Material> material, size_t numVertices, const void* vertexData);

		// Draw sprites takes a single vertex per sprite, duplicates the data across multiple vertices, and draws
		// vertPosOffset is the offset, in bytes, from the start of each vertex's data, to a Vector2f which will be filled with the vertex's position in 0-1 space.
		void drawSprites(std::shared_ptr<Material> material, size_t numSprites, const void* vertexData);

		// Draw one sliced sprite. Slices -> x = left, y = top, z = right, w = bottom, in [0..1] space relative to the texture
		void drawSlicedSprite(std::shared_ptr<Material> material, Vector2f scale, Vector4f slices, const void* vertexData);

		size_t getNumDrawCalls() const { return nDrawCalls; }
		size_t getNumVertices() const { return nVertices; }
		size_t getNumTriangles() const { return nTriangles; }

	protected:
		virtual void doStartRender() = 0;
		virtual void doEndRender() = 0;
		virtual void setVertices(MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices) = 0;
		virtual void drawTriangles(size_t numIndices) = 0;

		virtual void setViewPort(Rect4i rect, bool enableScissor) = 0;

	private:
		void bind(RenderContext& context);

		RenderContext* activeContext = nullptr;
		Matrix4f projection;
		Rect4i viewPort;
		Camera* camera = nullptr;

		size_t verticesPending = 0;
		size_t bytesPending = 0;
		size_t indicesPending = 0;
		Vector<char> vertexBuffer;
		Vector<unsigned short> indexBuffer;
		std::shared_ptr<Material> materialPending;

		size_t nDrawCalls = 0;
		size_t nVertices = 0;
		size_t nTriangles = 0;

		Vector<unsigned short> stdQuadIndexCache;

		void resetPending();
		void startDrawCall(std::shared_ptr<Material>& material);
		void flushPending();
		void executeDrawTriangles(Material& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices);

		void makeSpaceForPendingVertices(size_t numBytes);
		void makeSpaceForPendingIndices(size_t numIndices);
		PainterVertexData addDrawData(std::shared_ptr<Material>& material, size_t numVertices, size_t numIndices);

		unsigned short* getStandardQuadIndices(size_t numQuads);
		void generateQuadIndices(unsigned short firstVertex, size_t numQuads, unsigned short* target);
		void generateQuadIndicesOffset(unsigned short firstVertex, unsigned short lineStride, unsigned short* target);
	};
}
